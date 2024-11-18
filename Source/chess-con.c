/////////////////////////////////////////////////////////////////////////////////////
//
// CHESS
// =====
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
//
// Simple look ahead chess programme
//
/////////////////////////////////////////////////////////////////////////////////////
//
// Revision History
//
// 0.1	  Genesis
// 0.7    Finally working
// 0.71   Pawn->King at board end
// 0.72   Add Undo
// 0.75   BoardScore: Add count moves and threats
// 0.76   Display improvements
// 0.9    Added:
//          Simple Analysis parameter
//          Help
//          King taken => force redo move OR game end
//          ^Play command
// 0.91   MovePiece: bug fix
// 0.92   Add Castling (need to check for Check)
// 0.93   Added InCheck ()
//              Checked bit in _Piece
//              Moved count in _Piece
//        Castling now complient
//
/////////////////////////////////////////////////////////////////////////////////////


const char AppName [] = "Chess for Console";
const char Revision [] = "1.00";

#ifdef _Windows
  #include "..\Lib\Lib.c"
  #include "..\Lib\Console.c"
  #include "..\Lib\ConsoleLib.c"
  #include "Chess.c"
#else
  #include "../Lib/Lib.c"
  #include "../Lib/Console.c"
  #include "../Lib/ConsoleLib.c"
  #include "Chess.c"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Show Board and Input a valid move from the human
//

bool StrGetCoords (char **St, _Coord *Pos)
  {
    Pos->x = -1;
    Pos->y = -1;
    while (true)
      {
        StepSpace (St);
        if (IsAlpha (**St))
          if (Pos->x < 0)
            Pos->x = UpCase (**St) - 'A';
          else
            return false;
        else if (IsDigit (**St))
          if (Pos->y < 0)
            Pos->y = **St - '1';
          else
            return false;
        else
          return false;
        (*St)++;
        if ((Pos->x >= 0) && (Pos->y >= 0))
          {
            if ((Pos->x < 8) && (Pos->y < 8))
              return true;
            return false;
          }
      }
  }

char *StWho [] = {"YOU", "I"};

char *PieceSymbol [] = {"  ", "Ki", "Qu", "Ro", "Bi", "Kn", "p"}; //" KQRBKp";
//char PieceSymbol2 [] = " iuoin ";

_Coord Highlight = {-1, -1};
char Bodies [2] [64] = {"  ", "  "};

void ShowPieceTaken (_Piece p)
  {
    if (Piece (p) != pEmpty)
      {
        PutString ("  **");
        PutString (PieceSymbol [Piece (p)]);
        PutString (" Taken");
        StrConcat (Bodies [PieceWhite (p)], " ");
        StrConcat (Bodies [PieceWhite (p)], PieceSymbol [Piece (p)]);
        if (Piece (p) == pKing)
          {
            PutString ("  **");
            PutString (StWho [PieceWhite (p) == PlayerWhite]);
            PutString (" WON");
            GameOver = true;
          }
      }
  }

bool Cheat = false;
int MoveCount;

void ShowHelp ()
  {
    PutStringCRLF ("Help");
    PutStringCRLF ("  Enter a move by coordinates:");
    PutStringHighlight ("    example - |a2a4| means move the piece in column |a| row |2| to row |4|", ColYellow);
    PutNewLine ();
    PutStringHighlight ("    To |castle|, move the king |2 spaces| left or right", ColYellow);
    PutNewLine ();
    PutStringHighlight ("  |^Q| to quit", ColYellow);
    PutNewLine ();
    PutStringHighlight ("  |^Z| to undo your last move", ColYellow);
    PutNewLine ();
    PutStringHighlight ("  |^P| Play for me. I'm too dumb to", ColYellow);
  }

_Piece BoardPrev [8][8];   // Board before human movefor backup
bool BoardPrevOK = false;
int BodiesLen [2];

void Undo (void)
  {
    if (BoardPrevOK)
      {
        MemMove (Board, BoardPrev, sizeof (Board));
        BoardPrevOK = false;
        Bodies [false][BodiesLen [false]] = 0;
        Bodies [true][BodiesLen [true]] = 0;
        Highlight.x = -1;
      }
  }

void PutPos (_Coord Pos)
  {
    PutChar (Pos.x + 'a');
    PutChar (Pos.y + '1');
  }

void ConsoleColours (int FG, int BG)
  {
    ConsoleColourFG (FG);
    ConsoleColourBG (BG);
  }

void BoardShow (void)
  {
    int x, y;
    int c;
    //
    for (y = 8; y >= -1; y--)
      {
        PutNewLine ();
        PutString ("  ");
        if ((y == 8) || (y == -1))
          {
            PutString ("  ");
            for (x = 0; x < 8; x++)
              {
                PutChar (' ');
                PutChar ('a' + x);
                PutChar (' ');
              }
          }
        else
          {
            PutChar ('1' + y);
            PutChar (' ');
            for (x = 0; x < 8; x++)
              {
                if ((x ^ y) & 0x01)
                  ConsoleColourBG (ColGreenDark); //(ColBlack | ColBright);
                else
                  ConsoleColourBG (ColBrown); //(ColCyanDark);
                if (PieceWhite (Board [x][y]))
                  c = ColWhite | ColBright;
                else
                  c = ColBlack;
                if ((x == Highlight.x) && (y == Highlight.y))
                  c = c | ColItalic;
                ConsoleColourFG (c);
                PutChar (' ');
                PutStringN (PieceSymbol [Piece (Board [x][y])], 2);
              }
            ConsoleColours (ColWhite, ColBlack);
            PutChar (' ');
            PutChar ('1' + y);
            if (y == 7)
              PutString (Bodies [true]);
            if (y == 0)
              PutString (Bodies [false]);
          }
      }
  }

_SpecialMove MovePiece_ (_Coord From, _Coord To)
  {
    _SpecialMove sm;
    //
    sm = MovePiece (From, To);
    if (sm == smCastle)
      PutString ("  **Castled");
    else if (sm == smCrown)
      PutString ("  **Crowned");
    return sm;
  }

/////////////////////////////////////////////////////////////////////
// GetMove - Input a valid move
//
// Returns true if move is legal, false if human is quitting

bool GetMove (void)
  {
    char Command [80], *c;
    int ch;
    _Coord a, b;
    _Piece p;
    bool OK;
    //
    Command [0] = 0;
    OK = false;
    while (!OK && !GameOver)
      {
        PutNewLine ();
        PutChar ('[');
        PutInt (MoveCount, 0);
        PutChar (']');
        if (InCheck (PlayerWhite))
          PutString (" **CHECK");
        PutString (" Enter Move (F2=Help): ");
        while ((ch = EditString (Command, 80, 40)) < 0)
          ;   // don't do anything on a screen resize
        PutString (Command);
        if (ch == cr)
          {
            c = Command;
            if (StrGetCoords (&c, &a))
              if (StrGetCoords (&c, &b))
                if (*c == 0)
                  if (MoveValid (a, b) || Cheat)
                    {
                      MemMove (BoardPrev, Board, sizeof (Board));
                      BoardPrevOK = true;
                      BodiesLen [false] = StrLength (Bodies [false]);
                      BodiesLen [true] = StrLength (Bodies [true]);
                      p = Board [b.x][b.y];
                      MovePiece_ (a, b);
                      if (InCheck (PlayerWhite))
                        {
                          MemMove (Board, BoardPrev, sizeof (Board));
                          PutString (" ** Save the King");
                        }
                      else
                        {
                          OK = true;
                          ShowPieceTaken (p);
                        }
                    }
            if (!OK)
              ConsoleBeep ();
          }
        else if (ch == Cntrl ('P'))
          {
            PutString ("Play ");
            if (BestMove (PlayerWhite, 0) == MININT)   // no moves possible
              PutString (" ** NO MOVES. Give up");
            else
              {
                a = BestA [0];
                b = BestB [0];
                PutPos (a);
                PutPos (b);
                MemMove (BoardPrev, Board, sizeof (Board));
                BoardPrevOK = true;
                BodiesLen [false] = StrLength (Bodies [false]);
                BodiesLen [true] = StrLength (Bodies [true]);
                ShowPieceTaken (Board [b.x][b.y]);
                MovePiece_ (a, b);
                OK = true;
              }
          }
        else if (ch == Cntrl ('Z') && BoardPrevOK)
          {
            PutString ("Undo");
            Undo ();
            BoardShow ();
            MoveCount -=2;
          }
        else if ((ch == Cntrl ('C') || (ch == Cntrl ('Q'))))
          {
            PutString ("Quit");
            GameOver = true;
          }
        else if (ch == KeyF1+1) //Cntrl ('H'))
          ShowHelp ();
        else
          ConsoleBeep ();
      }
    return OK;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// main ()
// ====
//
// Parameters:
//   W   play as White
//   B   play as Black
//   C   Cheat: don't check human's moves
//   0-9 Set Depth
//   S   Simple analysis mode

int main (int argc, char *argv [])
  {
    int i;
    bool Show;
    _Coord a, b;
    _Piece p;
    int Score;
    int Time;
    //
    ConsoleInit (false);
    ConsoleClear (ColWhite, ColBlack);
    PutStringCRLF ("=========================");
    PutStringCRLF ("Stewart's Chess Programme");
    PutString     ("Rev ");
    PutStringCRLF (Revision);
    PutStringCRLF ("=========================");
    PlayerWhite = true;
    DepthPlay = 2;
    MoveCount = 1;
    GameOver = false;
    BoardInit ();
    for (i = 1; i < argc; i++)
      if (UpCase (*argv [i]) == 'W')
        PlayerWhite = true;
      else if (UpCase (*argv [i]) == 'B')
        PlayerWhite = false;
      else if (IsDigit (argv [i][0]))
        DepthPlay = argv [i][0] - '0';
      else if (UpCase (*argv [i]) == 'C')
        Cheat = true;
      else if (UpCase (*argv [i]) == 'S')
        SimpleAnalysis = true;
      else
        PutStringCRLF ("Invalid Parameter. Valid parameters: W B C 0-9 S");
    PutString ("Depth ");
    PutInt (DepthPlay, 0);
    if (SimpleAnalysis)
      PutString (" - Simple Analysis");
    PutNewLine ();
    Show = true;
    while (!GameOver)
      {
        if ((MoveCount & 0x01) == PlayerWhite)   // Human's Turn
          {
            if (Show)
              BoardShow ();
            Show = false;
            if (GetMove ())
              MoveCount++;
          }
        if (!GameOver && (MoveCount & 0x01) != PlayerWhite)   // CPU's Turn
          {
            PutNewLine ();
            MovesConsidered = 0;
            Time = ClockMS ();
            InCheck (!PlayerWhite);   // update Checked status on King piece
            Score = BestMove (!PlayerWhite, 0);
            if (Score == MININT)
              {
                PutStringCRLF ("I SURRENDER");
                GameOver = true;
              }
            else
              {
                a = BestA [0];
                b = BestB [0];
                Highlight = b;
                if (Piece (Board [b.x][b.y]) == pKing)   // I just took your King
                  {
                    PutString ("** By the way, you are in CHECK. Try again");
                    Undo ();
                    MoveCount--;
                  }
                else
                  {
                    p = Board [b.x][b.y];
                    PutChar ('[');
                    PutInt (MoveCount, 0);
                    PutString ("] ");
                    PutPos (a);
                    PutPos (b);
                    ShowPieceTaken (p);
                    MovePiece_ (a, b);
                    PutString ("  ");
                    PutInt (MovesConsidered, 0 | IntToLengthCommas);
                    PutString (" Moves. Score ");
                    PutInt (Score, 0 | IntToLengthCommas);
                    PutString (". Time ");
                    PutIntDecimals (ClockMS () - Time, 3);
                    MoveCount++;
                    Show = true;
                  }
              }
          }
      }
    PutNewLine ();
    ConsoleUninit (false);
  }

