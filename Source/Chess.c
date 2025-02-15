////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PLAY CHESS ENGINE
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024, 2025 Stewart Tunbridge, Pi Micros
//
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
  {
    int x, y;
  } _Coord;

typedef enum {pEmpty, pKing, pQueen, pRook, pBishop, pKnight, pPawn, pWhite = 0x08, pChecked = 0x10, pPawn2 = 0x20, pMoveID = 0x100} _Piece;
  // Bits 0-2 specifies piece, Bit 3 => piece white, Bit 4 => has been in check (King only), Bit 5 => double move (Pawn only)
  // Bits 8..31 Move ID: 0 => never moved.  (needed for castling & en passant

typedef _Piece _Board [8][8] ;   // Board [x][y]
_Board Board;   // This is THE BOARD
int MoveID;   // ID incremented every move

bool PlayerWhite;
int DepthPlay = 3;
bool SimpleAnalysis = false;
int Randomize = 0;

#define DepthMax 10

#define Piece(p)       (_Piece) (p & (pWhite-1))
#define PieceWhite(p)  ((p&pWhite) != 0)
#define PieceFrom(BasePiece,White) (_Piece)(White ? (BasePiece | pWhite) : BasePiece)


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BoardInit: Initialize the Board
//

_Piece BoardStart [8] = {pRook, pKnight, pBishop, pQueen, pKing, pBishop, pKnight, pRook};

void BoardInit ()
  {
    int x, y;
    //
    MoveID = 0;
    for (y = 0; y < 8; y++)
      for (x = 0; x < 8; x++)
        if (y == 0)
          Board [x][y] = PieceFrom (BoardStart [x], true);
        else if (y == 1)
          Board [x][y] = PieceFrom (pPawn, true);
        else if (y == 6)
          Board [x][y] = pPawn;
        else if (y == 7)
          Board [x][y] = BoardStart [x];
        else
          Board [x][y] = pEmpty;
    srand (time (NULL));   // Initialize random number generator
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

void GetPieceMoves (_Coord From, _Coord *Res)   // Res: array of _Coord
  {
    int Index;
    _Piece FromPce, ToPce, p_;
    _Coord *Moves;
    _Coord To;
    bool Repeat;
    bool Bad, PieceTaken;
    //
    FromPce = Board [From.x][From.y];
    Moves = PieceMoves [Piece (FromPce)];
    Repeat = PieceMovesRepeat [Piece (FromPce)];
    Index = 0;
    To = From;
    // Build list of all possible legal moves
    while (true)
      {
        if (Moves [Index].x == End)
          break;
        //if (!Repeat)
        //  *To = From;
        To.x += Moves [Index].x;
        if (PieceWhite (FromPce))   // Black pieces have reverse Y direction (only effects Pawns)
          To.y += Moves [Index].y;
        else
          To.y -= Moves [Index].y;
        Bad = false;
        PieceTaken = false;
        if ((To.x > 7) || (To.x < 0) || (To.y > 7) || (To.y < 0))   // off the board
          Bad = true;
        else
          {
            ToPce = (_Piece) Board [To.x][To.y];
            if (Piece (ToPce) != pEmpty)   // destination  NOT empty AND is
              if (PieceWhite (ToPce) == PieceWhite (FromPce))   // my own piece
                Bad = true;
              else
                PieceTaken = true;   // takes oponents piece
            if (Piece (FromPce) == pPawn)   // Pawn has more limitations
              switch (Index)
                {
                  case 0: // straight, double step
                          if (From.y != PawnFirstY [PieceWhite (FromPce)])   // not pawns start row
                            Bad = true;
                          else
                            if (Piece (Board [From.x][(From.y + To.y) / 2]) != pEmpty)   // also test square being jumped over
                              Bad = true;
                          // continue into Index == 1
                  case 1: // straight, single step OR double step
                          if (Piece (ToPce) != pEmpty)
                            Bad = true;
                          break;
                  case 2: // diagonal attacks
                  case 3: //
                          if (Piece (ToPce) == pEmpty)
                            {
                              p_ = Board [To.x][From.y];   // piece you moved behind -
                              if ( (Piece (p_) != pPawn) ||   // - not opponents pawn
                                   (PieceWhite (p_) == PieceWhite (FromPce)) ||
                                   ((p_ & pPawn2) == 0) ||   // - not there by double move
                                   (p_ / pMoveID != MoveID / pMoveID) )   // - not the last move
                                Bad = true;   // not en passan
                            }
                }
            else if (Piece (FromPce) == pKing)
              if (Index < 2)   // Castling
                if ((FromPce / pMoveID > 0) || (FromPce & pChecked))   // moved OR been in check?
                  Bad = true;
                else
                  {
                    p_ = PieceFrom (pRook, PieceWhite (FromPce));   // corresponding Rook for the castle: pRook, Colour, no moves
                    if (Piece (Board [(From.x + To.x) / 2][From.y]) != pEmpty)   // passing square must be empty
                      Bad = true;
                    else
                      if (Index == 0)   // Kingside castle
                        {
                          if (Board [7][To.y] != p_)
                            Bad = true;
                        }
                      else   // Index == 1   Queenside castle
                        if (Board [0][To.y] != p_)
                          Bad = true;
                        else if (Board [1][To.y] != pEmpty)
                          Bad = true;
                  }
          }
        if (Bad)   // no good, try next option
          {
            Index++;
            To = From;
          }
        else
          {
            *Res = To;
            Res++;
            if (!Repeat || PieceTaken)
              {
                Index++;
                To = From;
              }
          }
      }
    Res->x = -1;
    Res->y = -1;
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
    _Coord p1;
    _Coord Moves [64], *m;
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
                  GetPieceMoves (p1, Moves);
                  m = Moves;
                  while (m->x >= 0)
                    {
                      ds += 100; //a move = 1/10 of a pawn   (was ds++)
                      p_ = Piece (Board [m->x][m->y]);
                      if (p_ != pEmpty)   // Add Piece Value / 10 that you can attack
                        ds += Min (PieceValue [p_], PieceValue [pQueen]) / 4;  // was / 10
                      m++;
                    }
                }
              if (PieceWhite (p) == PlayWhite)
                Score += ds;
              else
                Score -= ds;
            }
        }
    if (Randomize)
      Score += rand () % (Randomize + Randomize + 1) - Randomize;
    return Score;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BestMove: Calculate best move recursively
//
// Returns Score of best move

typedef enum {smNone, smCrown, smCastle, smEnPassant} _SpecialMove;

int LastRow [] = {0, 7};
int PawnSkipRow [] = {5, 2};   // Row missed when pawns start with a double

// Move a piece allowing for special moves

_SpecialMove MovePiece (_Coord From, _Coord To)
  {
    _SpecialMove Res;
    _Piece Pce;
    //
    Res = smNone;
    MoveID += pMoveID;   // Next ID
    Pce = (_Piece) ((Board [From.x][From.y] & (pMoveID - 1 - pPawn2)) | MoveID);   // Set new MoveID and clear Pawn double move flag
    Board [From.x][From.y] = pEmpty;
    // Pawn special moves
    if (Piece (Pce) == pPawn)
      {
        // Check for double first move
        if (Abs (From.y - To.y) > 1)
          Pce = (_Piece) (Pce | pPawn2);   // set Pawn double move flag
        // Check for pawn reaching the far end of the board
        if (To.y == LastRow [PieceWhite (Pce)])   // Last row reached
          {
            Pce = (_Piece) (Pce - pPawn + pQueen);   // upgradde from Pawn to Queen
            Res = smCrown;
          }
        // Check for en passan
        else if (To.y == PawnSkipRow [!PieceWhite (Pce)])   // Up to skip row of opponent
          if (From.x != To.x)   // diagonal move
            if (Piece (Board [To.x][To.y]) == pEmpty)   // to an empty square. EN PASSANT
              {
                Board [To.x][From.y] = pEmpty;   // take piece (pawn) behind
                Res = smEnPassant;
              }
      }
    // Check for Castling
    else if (Piece (Pce) == pKing)
      {
        if (To.x == From.x + 2)   // kingside castle
          {
            Board [5][To.y] = Board [7][To.y];   // jump Rook
            Board [7][To.y] = pEmpty;
            Res = smCastle;
          }
        else if (To.x == From.x - 2)   // queenside castle
          {
            Board [3][To.y] = Board [0][To.y];   // jump Rook
            Board [0][To.y] = pEmpty;
            Res = smCastle;
          }
      }
    Board [To.x][To.y] = Pce;
    return Res;
  }

// undo the above

void UnmovePiece (_Coord From, _Coord To, _Piece OldFrom, _Piece OldTo, _SpecialMove SpecialMove)
  {
    Board [From.x][From.y] = OldFrom;
    Board [To.x][To.y] = OldTo;
    // Special moves
    MoveID -= pMoveID;   // Next ID
    if (SpecialMove == smCastle)   // return Rook
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
    else if (SpecialMove == smEnPassant)   // reinstate opponents pawn with the flags it would have had
      Board [To.x][From.y] = (_Piece) (PieceFrom (pPawn, !PieceWhite (OldFrom)) | pPawn2 | MoveID);
  }

longint MovesConsidered;
_Coord BestA [DepthMax], BestB [DepthMax];
//_Coord BestA_ [DepthMax], BestB_ [DepthMax];   // copy best move sequence

int BestMove (bool PlayWhite, int Depth)
  {
    _Coord Moves [64], *m;
    _Piece p, p_;
    _Coord From;
    _SpecialMove sm;
    int Score;
    int BestScore;
    //
    BestScore = MININT;
    // Go thru all my pieces and all their moves
    for (From.y = 0; From.y < 8; From.y++)   // for every square
      for (From.x = 0; From.x < 8; From.x++)
        {
          p = Board [From.x][From.y];
          if ((Piece (p) != pEmpty) && (PieceWhite (p) == PlayWhite))   // if my piece
            {
              GetPieceMoves (From, Moves);
              m = Moves;
              while (m->x >= 0)   // for all moves
                {
                  MovesConsidered++;
                  p_ = (_Piece) Board [m->x][m->y];   // piece being taken (or Empty)
                  if (Piece (p_) == pKing)   // This would end in victory
                    return MAXINT;   // so stop here report a win
                  sm = MovePiece (From, *m);
                  if (Depth == DepthPlay)   // Reached the limit of look-ahead
                    Score = BoardScore (PlayWhite);   // evaluate move
                  else   // otherwise find the reply move
                    Score = -BestMove (!PlayWhite, Depth + 1);
                  if (Score > BestScore)
                    {
                      BestScore = Score;
                      BestA [Depth] = From;
                      BestB [Depth] = *m;
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
                  UnmovePiece (From, *m, p, p_, sm);
                  m++;
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
    _Coord Moves [64], *m;
    //
    p = (_Piece) Board [From.x][From.y];
    if (Piece (p) != pEmpty)
      if (PieceWhite (p) == PlayerWhite)
        {
          GetPieceMoves (From, Moves);
          m = Moves;
          while (m->x >= 0)
            {
              if ((m->x == To.x) && (m->y == To.y))   // destination found
                return true;
              m++;
            }
        }
    return false;
  }

bool InCheck (bool PlayWhite)
  {
    _Coord From, Moves [64], *m;
    _Piece p, p_;
    //
    for (From.y = 0; From.y < 8; From.y++)   // for every square
      for (From.x = 0; From.x < 8; From.x++)
        {
          p = (_Piece) Board [From.x][From.y];
          if ((Piece (p) != pEmpty) && (PieceWhite (p) != PlayWhite))   // if not my piece
            {
              GetPieceMoves (From, Moves);
              m = Moves;
              while (m->x >= 0)   // for all moves
                {
                  p_ = Board [m->x][m->y];
                  if (Piece (p_) == pKing)   // and it can take my king
                    {
                      Board [m->x][m->y] = (_Piece) (p_ | pChecked);
                      return true;
                    }
                  m++;
                }
            }
        }
    return false;
  }
