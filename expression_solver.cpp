#include <math.h>

#include "memory.h"

#include "expression_solver.h"

double ExpressionSolver::solve(const TextLine &expression, int &offset) throw (InterpreterException)
{
  QList<int> result;
  _expression = expression;
  _startOffset = offset;
  _currentOffset = _startOffset;
  _numberStack.clear();
  _commandStack.clear();

  Token token, previousToken;
  while ((token = readToken()).tokenType() != Token::Type_EOF)
  {
    analyzeForSyntaxError(token, previousToken);

    if (token.tokenType() == Token::Type_Number ||
        token.isVariable() ||
        token.tokenType() == Token::Type_OpenArrayVar ||
        token.isPreFuncToken() ||
        token.isEntity(LCDChar_OpenParen))
      pushToken(token);
    else if (token.isOperatorToken())
    {
      if (!_commandStack.isEmpty())
      {
        if (isOperator(_commandStack.top().entity()) || isPreFunc(_commandStack.top().entity()))
        {
          if (comparePriorities(token.entity(), _commandStack.top().entity()) <= 0)
            performOperation(_commandStack.pop().entity());
        }
      }

      pushToken(token);
    } else if (token.isPostFuncToken())
    {
      if (!_commandStack.isEmpty())
      {
        if (isOperator(_commandStack.top().entity()) ||
            isPreFunc(_commandStack.top().entity()) ||
            isPostFunc(_commandStack.top().entity()))
        {
          if (comparePriorities(token.entity(), _commandStack.top().entity()) <= 0)
            performOperation(_commandStack.pop().entity());
        }
      }

      performOperation(token.entity());
    } else if (token.isEntity(LCDChar_CloseParen))
    {
      // Consume all operators
      performStackOperations();

      if (!_commandStack.isEmpty() && _commandStack.top().entity() == LCDChar_OpenParen)
        _commandStack.pop();
      else // No corresponding open parenthesis => syntax error
        throw InterpreterException(Error_Syntax, token.offset());
    } else if (token.isEntity(LCDChar_CloseBracket))
    {
      // Consume all operators
      performStackOperations(true, false);
      if (!_commandStack.isEmpty() && _commandStack.top().tokenType() == Token::Type_OpenArrayVar)
      {
        int entity = _commandStack.pop().entity();

        // Get the stack value, compute the array index and push it
        int index = (int) _numberStack.pop();
        bool overflow;
        _numberStack.push(Memory::instance().variable((LCDChar) entity, index, &overflow));
        if (overflow)
          throw InterpreterException(Error_Memory, token.offset());
      } else // Too much "]" => we stop the analyse and returns on the "]"
      {
        _currentOffset--;
        token = Token();
        break;
      }
    }

    previousToken = token;
  }

  analyzeForSyntaxError(token, previousToken);

  // Consume all resting operators
  performStackOperations(true, true);

  // Update offset
  offset = _currentOffset;

  return _numberStack.top();
}

void ExpressionSolver::performOperation(int entity) throw (InterpreterException)
{
  switch (entity)
  {
  case LCDChar_Multiply:
  case LCDChar_Divide:
  case LCDChar_Add:
  case LCDChar_Substract:
  case LCDOp_Xy:
  case LCDOp_xSquareRoot:
    {
      double d2 = _numberStack.pop();
      double d1 = _numberStack.pop();

      switch (entity)
      {
      case LCDChar_Multiply: _numberStack.push(d1 * d2); break;
      case LCDChar_Divide:
        if (d2 == 0.0)
          throw InterpreterException(Error_Math, _currentToken.offset());
        _numberStack.push(d1 / d2);
        break;
      case LCDChar_Add: _numberStack.push(d1 + d2); break;
      case LCDChar_Substract: _numberStack.push(d1 - d2); break;
      case LCDOp_Xy: _numberStack.push(pow(d1, d2)); break;
      case LCDOp_xSquareRoot: _numberStack.push(pow(d2, 1.0 / d1)); break; // NEED a real xSquareRoot function
      default:;
      }
      break;
    }
  // Prefixed functions
  case LCDChar_SquareRoot:
    {
      double d = _numberStack.pop();
      if (d < 0.0)
        throw InterpreterException(Error_Math, _currentToken.offset());
      _numberStack.push(sqrt(d));
    }
    break;
  case LCDOp_CubeSquareRoot: _numberStack.push(cbrt(_numberStack.pop())); break;
  case LCDOp_Log: _numberStack.push(log10(_numberStack.pop())); break;
  case LCDChar_Ten: _numberStack.push(pow(10.0, _numberStack.pop())); break;
  case LCDOp_Ln: _numberStack.push(log(_numberStack.pop())); break;
  case LCDChar_Euler: _numberStack.push(exp(_numberStack.pop())); break;
  case LCDOp_Sin: _numberStack.push(sin(native2rad(_numberStack.pop()))); break;
  case LCDOp_Cos: _numberStack.push(cos(native2rad(_numberStack.pop()))); break;
  case LCDOp_Tan: _numberStack.push(tan(native2rad(_numberStack.pop()))); break;
  case LCDOp_Sinh: _numberStack.push(sinh(_numberStack.pop())); break;
  case LCDOp_Cosh: _numberStack.push(cosh(_numberStack.pop())); break;
  case LCDOp_Tanh: _numberStack.push(tanh(_numberStack.pop())); break;
  case LCDOp_Sin_1: _numberStack.push(rad2deg(asin(_numberStack.pop()))); break;
  case LCDOp_Cos_1: _numberStack.push(rad2deg(acos(_numberStack.pop()))); break;
  case LCDOp_Tan_1: _numberStack.push(rad2deg(atan(_numberStack.pop()))); break;
  case LCDOp_Sinh_1: _numberStack.push(asinh(_numberStack.pop())); break;
  case LCDOp_Cosh_1: _numberStack.push(acosh(_numberStack.pop())); break;
  case LCDOp_Tanh_1: _numberStack.push(atanh(_numberStack.pop())); break;
  case LCDChar_MinusPrefix: _numberStack.push(-_numberStack.pop()); break;
  case LCDOp_Abs: _numberStack.push(fabs(_numberStack.pop())); break;
  case LCDOp_Int: _numberStack.push((int) _numberStack.pop()); break;
  case LCDOp_Frac: { double n = _numberStack.pop(); _numberStack.push(n - (double) ((int) n)); } break;
  case LCDChar_h: break;
  case LCDChar_d: break;
  case LCDChar_b: break;
  case LCDChar_o: break;
  case LCDOp_Neg: break;
  case LCDOp_Not: break;
  // Postfixed functions
  case LCDChar_Square: { double n = _numberStack.pop(); _numberStack.push(n * n); } break;
  case LCDChar_MinusOneUp: _numberStack.push(1.0 / _numberStack.pop()); break;
  case LCDChar_Exclamation: _numberStack.push(factorial(_numberStack.pop())); break;
  case LCDChar_DegSuffix: _numberStack.push(deg2native(_numberStack.pop())); break;
  case LCDChar_RadSuffix: _numberStack.push(rad2native(_numberStack.pop())); break;
  case LCDChar_GradSuffix: _numberStack.push(grad2native(_numberStack.pop())); break;
  case LCDChar_Degree: break;
  default:;
  }
}

void ExpressionSolver::performStackOperations(bool treatOpenParens, bool treatOpenBracket) throw (InterpreterException)
{
  while (!_commandStack.isEmpty())
  {
    if (_commandStack.top().isOperatorToken() ||
        _commandStack.top().isPreFuncToken() ||
        _commandStack.top().isPostFuncToken())
      performOperation(_commandStack.pop().entity());
    else if (_commandStack.top().isEntity(LCDChar_OpenParen) && treatOpenParens)
      _commandStack.pop();
    else if (_commandStack.top().tokenType() == Token::Type_OpenArrayVar && treatOpenBracket)
    {
      int entity = _commandStack.pop().entity();

      // Get the stack value, compute the array index and push it
      int index = (int) _numberStack.pop();
      bool overflow;
      _numberStack.push(Memory::instance().variable((LCDChar) entity, index, &overflow));
      if (overflow)
        throw InterpreterException(Error_Memory, 0); // WARNING
    }
    else
      break;
  }
}

Token ExpressionSolver::readToken() throw (InterpreterException)
{
  if (_currentOffset >= _expression.count())
  {
    _currentToken = Token(Token::Type_EOF, _expression.count());
    return _currentToken;
  }

  int entity = _expression[_currentOffset];
  switch (entity)
  {
  case LCDChar_OpenParen:
  case LCDChar_CloseParen:
  case LCDChar_CloseBracket: _currentToken = Token(entity, _currentOffset++); break;
  default:
    if (isOperator(entity) || isPreFunc(entity) || isPostFunc(entity))
      _currentToken = Token(entity, _currentOffset++);
    else if (isAlpha(entity))
    {
      _currentOffset++;
      if (_currentOffset < _expression.count() && _expression[_currentOffset] == LCDChar_OpenBracket)
      {
        Token token(Token::Type_OpenArrayVar, _currentOffset - 1);
        token.setEntity(_expression[_currentOffset++ - 1]);
        _currentToken = token;
      } else
        _currentToken = Token(_expression[_currentOffset - 1], _currentOffset - 1);
    } else if (isCipher(entity) || entity == LCDChar_Dot)
    {
      int firstOffset = _currentOffset;
      double d = parseNumber(_expression, _currentOffset);
      _currentToken = Token(Token::Type_Number, firstOffset);
      _currentToken.setValue(d);
    } else
      _currentToken = Token();
  }
  return _currentToken;
}

double ExpressionSolver::parseNumber(const TextLine &expression, int &offset) throw (InterpreterException)
{
  if (expression[offset] != LCDChar_Dot && !isCipher(expression[offset]))
    return 0.0;

  QString numberStr;
  numberStr.append(toNumChar(expression[offset++]));
  while (offset < expression.count() && (isCipher(expression[offset]) ||
         expression[offset] == LCDChar_Dot))
  {
    // Continue the number
    if (isCipher(expression[offset]))
      numberStr.append(toNumChar(expression[offset]));
    else if (numberStr.indexOf('.') < 0)
      numberStr.append('.');
    else
      throw InterpreterException(Error_Syntax, offset);
    offset++;
  }
  return numberStr.toDouble();
}

void ExpressionSolver::pushToken(const Token &token) throw (InterpreterException)
{
  if (token.tokenType() == Token::Type_Number || token.isVariable())
  {
    if (_numberStack.count() < _numberStackLimit)
      _numberStack.push(token.value());
    else
      throw InterpreterException(Error_Stack, token.offset());
  } else
  {
     if (_commandStack.count() < _commandStackLimit)
      _commandStack.push(token);
    else
      throw InterpreterException(Error_Stack, token.offset());
  }
}

void ExpressionSolver::analyzeForSyntaxError(Token token, Token previousToken) throw (InterpreterException)
{
  // Previous token constitancy
  if (previousToken.isOperatorToken() ||
      previousToken.isPreFuncToken() ||
      previousToken.isEntity(LCDChar_OpenParen) ||
      previousToken.tokenType() == Token::Type_OpenArrayVar)
  {
    if (token.isOperatorToken() ||
        token.isPostFuncToken() ||
        token.isEntity(LCDChar_CloseParen) ||
        token.isEntity(LCDChar_CloseBracket) ||
        token.tokenType() == Token::Type_EOF)
      throw InterpreterException(Error_Syntax, token.offset());
  }
  else if (previousToken.isPostFuncToken() ||
           previousToken.isEntity(LCDChar_CloseParen) ||
           previousToken.isVariable())
  {
    if (token.tokenType() == Token::Type_Number)
      throw InterpreterException(Error_Syntax, token.offset());
  }
}

bool ExpressionSolver::isExpressionStartEntity(int entity)
{
  return entity == LCDChar_OpenParen ||
         isPreFunc(entity) ||
         isCipher(entity) ||
         isAlpha(entity) ||
         entity == LCDChar_Dot;
}

double ExpressionSolver::numberStackTop(bool &empty)
{
  empty = !_numberStack.count();
  if (!empty)
    return _numberStack.top();
  else
    return 0.0;
}

void ExpressionSolver::emptyStacks()
{
  _numberStack.clear();
  _commandStack.clear();
}

double ExpressionSolver::native2rad(double native) const
{
  switch (CalculatorState::instance().angleMode())
  {
  case Deg: return deg2rad(native);
  case Rad: return native;
  default: return grad2rad(native); // Grad
  }
}

double ExpressionSolver::rad2native(double rad) const
{
  switch (CalculatorState::instance().angleMode())
  {
  case Deg: return rad2deg(rad);
  case Rad: return rad;
  default: return rad2grad(rad); // Grad
  }
}

double ExpressionSolver::deg2native(double deg) const
{
  switch (CalculatorState::instance().angleMode())
  {
  case Deg: return deg;
  case Rad: return deg2rad(deg);
  default: return deg2grad(deg); // Grad
  }
}

double ExpressionSolver::grad2native(double grad) const
{
  switch (CalculatorState::instance().angleMode())
  {
  case Deg: return grad2deg(grad);
  case Rad: return grad2rad(grad);
  default: return grad; // Grad
  }
}
