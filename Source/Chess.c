////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PLAY CHESS ENGINE
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
  {
    int x, y;
  } _Coord;

typedef enum {pEmpty, pKing, pQueen, pRook, pBishop, pKnight, pPawn, pWhite = 0x0008, pChecked = 0x0010, pMoves = 0x100} _Piece;
  // Bits 0-2 specify piece, Bit 3 specifies colour, Bit 4 goes true when in check (King only), Bits 8-- Counts the moves the piece has had
  // Everything past bit 3 is purely for legal castling.

typedef int _Board [8][8] ;   // Board [x][y]

_Board Board;
//_Piece Board [8][8];

bool SimpleAnalysis = false;

#define DepthMax 10

#define Piece(p)       (_Piece)(p&(pWhite-1))
#define PieceWhite(p)  ((p&pWhite)!=0)

bool PlayerWhite;
//bool GameOver;
int DepthPlay = 3;


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BoardInit: Initialize the Board
//

_Piece BoardStart [8] = {pRook, pKnight, pBishop, pQueen, pKing, pBishop, pKnight, pRook};

void BoardInit ()
  {
    int x, y;
    //
    for (y = 0; y < 8; y++)
      for (x = 0; x < 8; x++)
        if (y == 0)
          Board [x][y] = BoardStart [x] | pWhite;
        else if (y == 1)
          Board [x][y] = pPawn | pWhite;
        else if (y == 6)
          Board [x][y] = pPawn;
        else if (y == 7)
          Board [x][y] = BoardStart [x];
        else
          Board [x][y] = pEmpty;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PieceMove: Return all possible legal moves for piece at From.
//
// Resuls given in To. One at a time.
// Return false when no legal moves left

#define End 99999

_Coord PieceMovesKing [] = {{2, 0}, {-2, 0}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {End, End}};

_Coord PieceMovesAllDir [] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {End, End}};

_Coord PieceMovesRook [] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, {End, End}};

_Coord PieceMovesBishop [] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}, {End, End}};

_Coord PieceMovesKnight [] = {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}, {End, End}};

_Coord PieceMovesPawn [] = {{0, 2}, {0, 1}, {1, 1}, {-1, 1}, {End, End}};

//                          pEmpty, pKing,          pQueen,           pRook,          pBishop,          pKnight,          pPawn
int  PieceMovesRepeat [] = {false,  false,          true,             true,           true,             false,            false};
_Coord *PieceMoves [] =    {NULL,   PieceMovesKing, PieceMovesAllDir, PieceMovesRook, PieceMovesBishop, PieceMovesKnight, PieceMovesPawn};

int PawnFirstY [] = {6, 1};
int FirstY [] = {7, 0};

bool PieceMove (_Coord From, _Coord *To, int *Index)
  {
    _Piece FromPce, ToPce;
    _Coord *Moves;
    bool Repeat;
    _Piece pr;
    bool Bad;
    //
    FromPce = (_Piece) Board [From.x][From.y];
    Moves = PieceMoves [Piece (FromPce)];
    Repeat = PieceMovesRepeat [Piece (FromPce)];
    // Are we repeating and have already taken a piece? If so, go to next direction from (x0, y0)
    if (Repeat)
      if ((From.x != To->x) || (From.y != To->y))   // we have moved
        if (Piece (Board [To->x][To->y]) != pEmpty)   // current position is not empty
          {
            if (Moves [*Index].x != End)
              (*Index)++;
            *To = From;
          }
    // Next position
    while (true)
      {
        if (Moves [*Index].x == End)
          return false;
        if (!Repeat)
          *To = From;
        To->x += Moves [*Index].x;
        if (PieceWhite (FromPce))   // Black pieces have reverse Y direction (only effects Pawns)
          To->y += Moves [*Index].y;
        else
          To->y -= Moves [*Index].y;
        Bad = false;
        if ((To->x > 7) || (To->x < 0) || (To->y > 7) || (To->y < 0))   // off the board
          Bad = true;
        else
          {
            ToPce = (_Piece) Board [To->x][To->y];
            if (Piece (ToPce) != pEmpty)   // destination  NOT empty AND is
              if (PieceWhite (ToPce) == PieceWhite (FromPce))   // my own piece
                Bad = true;
            if (Piece (FromPce) == pPawn)   // Pawn has more limitations
              switch (*Index)
                {
                  case 0: // straight, double step
                          if (From.y != PawnFirstY [PieceWhite (FromPce)])   // not pawns start row
                            Bad = true;
                          else
                            if (Piece (Board [From.x][(From.y + To->y) / 2]) != pEmpty)   // also test square being jumped over
                              Bad = true;
                          // continue into *Index == 1
                  case 1: // straight, single step OR double step
                          if (Piece (ToPce) != pEmpty)
                            Bad = true;
                          break;
                  case 2: // diagonal attacks
                  case 3:
                          if (Piece (ToPce) == pEmpty)
                            Bad = true;
                }
            else if (Piece (FromPce) == pKing)
              if (*Index < 2)   // Castling
                if ((FromPce / pMoves > 0) || (FromPce & pChecked))   // moved
                  Bad = true;
                else
                  {
                    pr = pRook;
                    if (PieceWhite (FromPce))
                      pr = (_Piece) (pr | pWhite);   // corresponding Rook for the castle: pRook, Colour, no moves
                    if (Piece (Board [(From.x + To->x) / 2][From.y]) != pEmpty)   // passing square must be empty
                      Bad = true;
                    else
                      if (*Index == 0)   // Kingside castle
                        {
                          if (Board [7][To->y] != pr)
                            Bad = true;
                        }
                      else   // *Index == 1   Queenside castle
                        if (Board [0][To->y] != pr)
                          Bad = true;
                        else if (Board [1][To->y] != pEmpty)
                          Bad = true;
                  }
          }
        if (Repeat)
          if (Bad)   // no good, try next option
            {
              (*Index)++;
              *To = From;
            }
          else
            return true;
        else   // mSingle | mPawn: only 1 step each direction
          {
            (*Index)++;
            if (!Bad)   // OK
              return true;
          }
      }
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BoardScore: Calculate Value of the Board for specified player
//
// 0 => Equal position both sides
// BoardScore (Player) = - BoardScore (!Player)

#define MAXINT (((unsigned int) -1)>>1)
#define MININT (-MAXINT)   // I know this is rong, but it prevents overflow when negating. Was (~MAXINT)

int PieceValue [] = {0,      10000000, 8800,   5100,  3330,    3200,    1000};
//                   pEmpty, pKing,    pQueen, pRook, pBishop, pKnight, pPawn
//                   Hans Berliners system (based on experience and computer experiments):

int BoardScore (bool PlayWhite)
  {
    _Coord p1, p2;
    int Index;
    _Piece p, p_;
    int ds;
    int Score;
    //
    Score = 0;
    for (p1.y = 0; p1.y < 8; p1.y++)
      for (p1.x = 0; p1.x < 8; p1.x++)
        {
          p = (_Piece) Board [p1.x][p1.y];
          if (Piece (p) != pEmpty)
            {
              ds = PieceValue [Piece (p)];   // Score piece value
              if (!SimpleAnalysis)
                {
                  // Add 1 for every available move
                  p2 = p1;
                  Index = 0;
                  while (PieceMove (p1, &p2, &Index))
                    {
                      ds++;
                      p_ = Piece (Board [p2.x][p2.y]);
                      if (p_ != pEmpty)   // Add Piece Value / 10 that you can attack
                        ds += Min (PieceValue [p_], PieceValue [pQueen]) / 10;
                    }
                }
              if (PieceWhite (p) == PlayWhite)
                Score += ds;
              else
                Score -= ds;
            }
        }
    return Score;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BestMove: Calculate best move recursively
//
// Returns Score of best move

typedef enum {smNone, smCrown, smCastle} _SpecialMove;

int LastRow [] = {0, 7};

// Move a piece allowing for special moves

_SpecialMove MovePiece (_Coord From, _Coord To)
  {
    int p;
    //
    p = Board [From.x][From.y] + pMoves;   // tag piece as moved
    Board [From.x][From.y] = pEmpty;
    Board [To.x][To.y] = p;
    // Check for pawn reaching the far end of the board
    if (Piece (p) == pPawn)
      {
        if (To.y == LastRow [PieceWhite (p)])   // Last row reached
          {
            Board [To.x][To.y] = p - pPawn + pQueen;
            return smCrown;
          }
      }
    // Check for Castling
    else if (Piece (p) == pKing)
      {
        if (To.x == From.x + 2)   // kingside castle
          {
            Board [5][To.y] = Board [7][To.y];   // jump Rook
            Board [7][To.y] = pEmpty;
            return smCastle;
          }
        if (To.x == From.x - 2)   // queenside castle
          {
            Board [3][To.y] = Board [0][To.y];   // jump Rook
            Board [0][To.y] = pEmpty;
            return smCastle;
          }
      }
    return smNone;
  }

// undo the above

void UnmovePiece (_Coord From, _Coord To, _Piece OldTo, _SpecialMove SpecialMove)
  {
    int p;
    // move back
    p = Board [To.x][To.y] - pMoves;
    Board [From.x][From.y] = p;
    Board [To.x][To.y] = OldTo;
    // Special moves
    if (SpecialMove == smCrown)   // uncrown
      Board [From.x][From.y] = p - pQueen + pPawn;
    else if (SpecialMove == smCastle)   // return Rook
      {
        if (To.x == 6)   // kingside castle
          {
            Board [7][To.y] = Board [5][To.y];
            Board [5][To.y] = pEmpty;
          }
        else   // queenside castle
          {
            Board [0][To.y] = Board [3][To.y];
            Board [3][To.y] = pEmpty;
          }
      }
  }

longint MovesConsidered;
_Coord BestA [DepthMax], BestB [DepthMax];
//_Coord BestA_ [DepthMax], BestB_ [DepthMax];   // copy best move sequence

int BestMove (bool PlayWhite, int Depth)
  {
    _Piece p, p_;
    _Coord From, To;
    _SpecialMove sm;
    int Index;
    int Score;
    int BestScore;
    //
    BestScore = MININT;
    // Go thru all my pieces and all their moves
    for (From.y = 0; From.y < 8; From.y++)   // for every square
      for (From.x = 0; From.x < 8; From.x++)
        {
          p = (_Piece) Board [From.x][From.y];
          if ((Piece (p) != pEmpty) && (PieceWhite (p) == PlayWhite))   // if my piece
            {
              To = From;
              Index = 0;
              while (PieceMove (From, &To, &Index))   // for all moves
                {
                  MovesConsidered++;
                  p_ = (_Piece) Board [To.x][To.y];   // piece being taken (or Empty)
                  if (Piece (p_) == pKing)   // This would end in victory
                    return MAXINT;   // so stop here report a win
                  sm = MovePiece (From, To);
                  if (Depth == DepthPlay)   // Reached the limit of look-ahead
                    Score = BoardScore (PlayWhite);   // evaluate move
                  else   // otherwise find the reply move
                    Score = -BestMove (!PlayWhite, Depth + 1);
                  if (Score > BestScore)
                    {
                      BestScore = Score;
                      BestA [Depth] = From;
                      BestB [Depth] = To;
                      /*for (Index = Depth; Index < SIZEARRAY (BestA); Index++)
                        {
                          BestA_ [Index] = BestA [Index];
                          BestB_ [Index] = BestB [Index];
                        }
                     if (Depth == 0)
                        {
                          memcpy (BestA_, BestA, sizeof (BestA_));
                          memcpy (BestB_, BestB, sizeof (BestB_));
                        }*/
                    }
                  UnmovePiece (From, To, p_, sm);
                }   // no more moves
            }
        }   // no more pieces
    return BestScore;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MoveValid: Returns true if move is legal
//

bool MoveValid (_Coord From, _Coord To)
  {
    _Piece p;
    _Coord Pos;
    int Index;
    //
    p = (_Piece) Board [From.x][From.y];
    if (Piece (p) != pEmpty)
      if (PieceWhite (p) == PlayerWhite)
        {
          Pos = From;
          Index = 0;
          while (PieceMove (From, &Pos, &Index))
            if ((Pos.x == To.x) && (Pos.y == To.y))   // destination found
              return true;
        }
    return false;
  }

bool InCheck (bool PlayWhite)
  {
    _Coord From, To;
    _Piece p;
    int Index;
    //
    for (From.y = 0; From.y < 8; From.y++)   // for every square
      for (From.x = 0; From.x < 8; From.x++)
        {
          p = (_Piece) Board [From.x][From.y];
          if ((Piece (p) != pEmpty) && (PieceWhite (p) != PlayWhite))   // if not my piece
            {
              To = From;
              Index = 0;
              while (PieceMove (From, &To, &Index))   // for all moves
                if (Piece (Board [To.x][To.y]) == pKing)   // and it can take my king
                  {
                    Board [To.x][To.y] |= pChecked;
                    return true;
                  }
            }
        }
      return false;
  }
