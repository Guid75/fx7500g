#ifndef EXPRESSION_SOLVER_H
#define EXPRESSION_SOLVER_H

#include <QStack>

#include "misc.h"
#include "token.h"

class ExpressionSolver
{
public:
  ExpressionSolver() {}

  // <offset> is the start offset in <expression> and will be written with the next offset to be read after the expression
  double solve(const TextLine &expression, int &offset) throw (InterpreterException);

  // Returns 0.0 if expression is not a number
  static double parseNumber(const TextLine &expression, int &offset) throw (InterpreterException);

  // Static methods
  static bool isExpressionStartEntity(int entity);

  void emptyStacks();
  double numberStackTop(bool &empty); // Return 0.0 is empty

private:
  static const int _numberStackLimit = 9;
  static const int _commandStackLimit = 20;
  TextLine _expression;
  QStack<double> _numberStack;
  QStack<Token> _commandStack;
  int _startOffset;
  int _currentOffset;
  Token _currentToken;

  // Returns a token of type Type_EOF if the token is not usable in expression (expression overflow, separator, unknown token
  Token readToken() throw (InterpreterException);

  void pushToken(const Token &token) throw (InterpreterException);

  void performStackOperations(bool treatOpenParens = false, bool treatOpenBracket = false) throw (InterpreterException);
  void performOperation(int entity) throw (InterpreterException);

  void analyzeForSyntaxError(Token token, Token previousToken) throw (InterpreterException);

  double native2rad(double native) const;
  double rad2native(double rad) const;
  double deg2native(double deg) const;
  double grad2native(double grad) const;
};

#endif
