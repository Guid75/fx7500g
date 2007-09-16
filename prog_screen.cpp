#include "program.h"

#include "prog_screen.h"

ProgScreen::ProgScreen() : TextScreen()
{
  setCursorMode(CursorMode_Space);
}

void ProgScreen::init(CalculatorState *calcState)
{
  TextScreen::init(calcState);
  connect(calcState, SIGNAL(sysModeChanged(SysMode)), this, SLOT(sysModeChanged(SysMode)));
}

void ProgScreen::moveLeft()
{
  if (_cursorCol <= _startCursorCol)
    moveCursor(_startCursorCol + 9, _cursorLine);
  else
    moveCursor(_cursorCol - 1, _cursorLine);
}

void ProgScreen::moveRight()
{
  if (_cursorCol >= _startCursorCol + 9)
    moveCursor(_startCursorCol, _cursorLine);
  else
    moveCursor(_cursorCol + 1, _cursorLine);
}

void ProgScreen::feedScreen()
{
  clear();

  assignToScreen(LCDString(QString("sys mode : %1").arg(_calcState->sysModeString())), 0, 0);

  if (_calcState->calMode() == CalMode_BASE_N)
  {
    assignToScreen(LCDString("cal mode :BASE-N"), 0, 1);
    assignToScreen(LCDString(QString("            %1").arg(_calcState->baseModeString())), 0, 2);
  } else
  {
    assignToScreen(LCDString(QString("cal mode : %1").arg(_calcState->calModeString())), 0, 1);
    assignToScreen(LCDString(QString("   angle : %1").arg(_calcState->angleModeString())), 0, 2);
    assignToScreen(LCDString(QString(" display : %1").arg(_calcState->displayModeString())), 0, 3);
  }

  QString strBytesFree = QString("%1 Bytes Free").arg(Programs::instance().freeSteps());
  assignToScreen(LCDString(QString(16 - strBytesFree.length(), ' ') + strBytesFree), 0, 5);

  QString str = " Prog 0123456789";

  Programs &programs = Programs::instance();
  for (int i = 0; i < programs.count(); ++i)
    if (!programs.at(i).isEmpty())
      str[6 + i] = '_';
  assignToScreen(LCDString(str), 0, 7);
}

void ProgScreen::sysModeChanged(SysMode oldMode)
{
  feedScreen();

  if (oldMode != SysMode_WRT && oldMode != SysMode_PCL)
  {
    _cursorCol = _startCursorCol;
    _cursorLine = _startCursorLine;
    restartBlink();
  }
}

void ProgScreen::buttonClicked(int button)
{
  switch (_calcState->sysMode())
  {
  case SysMode_WRT:
    switch (button)
    {
    case Button_Exe: emit editProgram(currentProgramIndex()); break;
    default:;
    }
    break;
  case SysMode_PCL: // Only pad 2 is necessary for PCL mode
    switch (button)
    {
    case Button_Ac:
      Programs::instance().clear(currentProgramIndex());
      feedScreen();
      emit screenChanged();
      break;
    case Button_Del:
      if (_calcState->keyMode() == KeyMode_Shift ||
          _calcState->keyMode() == KeyMode_ShiftMode ||
          _calcState->keyMode() == KeyMode_ShiftHyp)
      {
        Programs::instance().clearAll();
        feedScreen();
        emit screenChanged();
      }
      break;
    default:;
    }
    break;
  default:;
  }

  switch (button)
  {
  case Button_Left: moveLeft(); break;
  case Button_Right: moveRight(); break;
  default:;
  }

  restartBlink();
}

int ProgScreen::currentProgramIndex() const
{
  return _cursorCol - _startCursorCol;
}