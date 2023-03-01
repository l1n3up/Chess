#include<iostream>
#include<vector>
#include<tuple>
#include <iomanip> 
#include <algorithm> 


using namespace std;
 
typedef unsigned char uchar;
 
class __Chess {//main game class
 
  private:
 
    enum {
        EMPTY,
        PAWN, QUEEN, KING,
        KNIGHT, BISHOP, ROOK,
        BARRIER
    };//enumeration figures and borders
    enum {
        ANGLE0,//up, down
        ANGLE45,//45 degree angle
        ANGLE90,//left, right
        ANGLE135//135 degree angle
    };
 
    typedef struct {
        uchar figure;//figure variable
        uchar mobility;//direction of movement
        uchar color;//cell color
        uchar attacked;//danger for king
        uchar special;//bounded moves
    } cell_t;
 
    cell_t **desk;
    char **move_table;//history of moves
    int moveptr_list[6] = { 0, 0, 0, 0, 0, 0 };
    struct {
        int fromX, fromY, toX, toY, figure;//figure coordinates
    } current_move;
    int side;//1 for white and 0 for black
    int castling_active[2][3];//castling(2 kings and 4 rooks)
 
    struct {
        int exist, X, Y;
    } extra;//Taking on the aisle
 
    typedef void (__Chess::*line_handler_t)(int, int, int, int);//pointer on move generation function
    typedef void(__Chess::*cell_handler_t)(int, int);//move processor pointer
    //(moves establishing , castling and first move)
    const char *starting =
        "rgbqkbgr"//white figures
        "pppppppp"
        "........"
        "........"
        "........"
        "........"
        "PPPPPPPP"
        "RGBQKBGR"//black figures
        "000000e8e8k";//000000 - castling condition, e8e8k - last move
 
    vector<tuple<int, int, int, int>> moves;//vector from tuples type int
 
    int white_moves = 0, black_moves = 0;
 
    void clear_desk() {//desk clearing
        for (int i = 1; i <= 8; ++i) {
            for (int j = 1; j <= 8; ++j) {
                desk[i][j].figure = EMPTY; //cell clearing 
            }
        }
    }
    void clear_desk_state() {
        for (int i = 1; i <= 8; ++i) {
            for (int j = 1; j <= 8; ++j) {
                desk[i][j].attacked = 0;//king is not attacked
                desk[i][j].special = 0;//no special moves
            }
        }
    }
    void init_desk() {
        desk = new cell_t* [10];//memory for structure pointer
        for (int i = 0; i < 10; ++i) {
            desk[i] = new cell_t [10];//memory for desk
        }
        for (int i = 0; i < 10; ++i) {//desk border establishment
            desk[i][0].figure = BARRIER;
            desk[i][9].figure = BARRIER;
            desk[0][i].figure = BARRIER;
            desk[9][i].figure = BARRIER;
        }
        clear_desk();
        clear_desk_state();
    }
 
    void clear_move_table() {//cleaning the history of moves
        for (int i = 0; i < 6; ++i) {
            moveptr_list[i] = 0;
        }
    }
    void init_move_table() {//move desk innitialization
        move_table = new char* [6];//memory for 6 rows
        for (int i = 0; i < 6; ++i)
            move_table[i] = new char[1024];//memory for array with moves
        clear_move_table();
    }
 
    void destroy_move_table() {//clear desk with moves
        for (int i = 0; i < 6; ++i) {
            delete move_table[i];
        }
        delete move_table;
    }
 
    void add_move(int fromX, int fromY, int toX, int toY) {//move adding function
        char *move_list = move_table[desk[fromX][fromY].figure - 1];//saving moves in list
        int moveptr = moveptr_list[desk[fromX][fromY].figure - 1];//move list pointer
        move_list[moveptr++] = fromX + 'a' - 1;//writing moves in moves list and conv to char
        move_list[moveptr++] = fromY + '0';
        move_list[moveptr++] = toX + 'a' - 1;
        move_list[moveptr++] = toY + '0';
        moveptr_list[desk[fromX][fromY].figure - 1] = moveptr;
    }
    int is_any_move_exist() {
        for (int i = 0; i < 6; ++i) {
            if (moveptr_list[i])//if the row contains sth, you can move
                return 1;
        }
        return 0;
    }
 
    void set_current_move(int fromX, int fromY, int toX, int toY, int figure) {//current code establishing
        current_move.fromX = fromX;
        current_move.fromY = fromY;
        current_move.toX = toX;
        current_move.toY = toY;
        current_move.figure = figure;
    }
    void release_current_move() {
        int fromX = current_move.fromX,
            fromY = current_move.fromY,
            toX = current_move.toX,
            toY = current_move.toY,
            figure = current_move.figure;
        side = !desk[fromX][fromY].color;//side changing
        if (fromX == toX && fromY == toY) {//if the move on the same position as was
            return;
        }
        update_castling();//castling
        extra_check();//taking on the pass
 
        if (desk[fromX][fromY].figure == KING && abs(fromX - toX) > 1) {//while castling:
            int _fromX = fromX < toX ? 8 : 1;//if fromX < toX castling h1(h8), if not, с - a1(а8)
            int _toX = (fromX + toX) / 2;//rook coordinates after the castling
            desk[_toX][toY] = desk[_fromX][fromY];//rook movement
            desk[_fromX][fromY].figure = EMPTY;//empty rook space
        }
        if (desk[fromX][fromY].figure == PAWN) {//pawn move
            if (toY == (side ? 1 : 8)) {//if white, 1st line, if black 8 line
                desk[fromX][fromY].figure = figure - 1;
            }
            if (fromX != toX && desk[toX][toY].figure == EMPTY) {
                desk[toX][fromY].figure = EMPTY;
            }
        }
        desk[toX][toY] = desk[fromX][fromY];//movement
        desk[fromX][fromY].figure = EMPTY;//cell becomes empty after the move
        side ? (++white_moves) : (++black_moves);
    }
    void update_castling() {//castling
        int fromX = current_move.fromX,//assigning coordinates into a variable
            fromY = current_move.fromY,
            toX = current_move.toX,
            toY = current_move.toY;
        for (int i = 1; i <= 8; i += 7) {
            for (int j = 1; j <= 8; j += 3 + (j == 1)) { 
                castling_active[i < 8][j / 4] |= (fromX == j && fromY == i);//
                castling_active[i < 8][j / 4] |= (toX == j && toY == i);
            }
        }
    }
 
    int extra_figure(const cell_t *cell) {
        if (cell->figure == EMPTY) {//if cell is empty
            return 0;
        }//continuing check
 
        if ((cell->color == side && cell->figure == KING) ||
                (cell->color != side && (cell->figure == ROOK || cell->figure == QUEEN))) {
            return cell->figure;//returning a cell with a figure on it
        } else {//
            return -1;//return in case an error occurs
        }
    }
    void extra_check() {//пtaking on the pass handler
        int fromX = current_move.fromX,//coordinates writing
            fromY = current_move.fromY,
            toX = current_move.toX,//move that can be done through 2 cells
            toY = current_move.toY;
        extra.exist = 0;
 
        //correctness of the move
        //taking on the pass existence check: fromY - toY = 2
        if (desk[fromX][fromY].figure != PAWN || abs(fromY - toY) < 2) {
            return;
        }
        int X, Y;//coordinates of pawn that can do taking on the pass
        //if the move was 2 cell forward and cell, through which move was done 
        //color corresponds figure that moves
 
        //left side check on taking on the pass
        if (desk[toX - 1][toY].figure == PAWN && desk[toX - 1][toY].color == side) {
            X = toX - 1;//becomes 1 cell lower
            Y = toY;
            //right side check on taking on the pass
        } else if (desk[toX + 1][toY].figure == PAWN && desk[toX + 1][toY].color == side) {
            X = toX + 1;////becomes 1 cell upper
            Y = toY;
        } else {//if no pawn- no taking on the pass
            return;
        }
 
        extra.exist = 1;//taking on the pass can be done
        extra.X = toX;//coordinates, which pawn going to get after taking on the pass
        extra.Y = toY + (side ? 1 : -1);//establishing the direction of move, if white + 1, move forwards on the desk
        int dec = 0, inc = 0;
 
        for (int i = X - 1; i > 0; --i) {//border check for the pawn
            int t = extra_figure(&desk[i][toY]);//assigning a cell with a figure to a variable
            //if t = 0 we continue to check
            if (t > 0) {//if figure exist 
                dec = t;//save
                break;
            } else if (t < 0) {
                return;
            }
        }
        for (int i = X + 1; i <= 8; ++i) {//border check 
            int t = extra_figure(&desk[i][toY]);//cell figure transferring
            if(t > 0) {//if figure exists
                inc = t;//figure saving
                break;
            } else if (t < 0) {
                return;
            }
        }
        if (inc && dec && (dec == KING || inc == KING)) {//if king, we cant move due to check
            extra.exist = 0;//set default value to the pass taking
        }
    }
 
    char get_figure_symbol(int figure) {//enum to char
        return (figure == PAWN) * 'p' + (figure == QUEEN) * 'q' + (figure == KING) * 'k'
               + (figure == KNIGHT) * 'g' + (figure == BISHOP) * 'b' + (figure == ROOK) * 'r'
               + (figure == EMPTY) * '.';
    }
    int get_figure_number(char figure) {//char to enum
        figure = tolower(figure);
        return (figure == 'p') * PAWN + (figure == 'q') * QUEEN + (figure == 'k') * KING
               + (figure == 'g') * KNIGHT + (figure == 'b') * BISHOP + (figure == 'r') * ROOK
               + (figure == '.') * EMPTY;
    }
    void unpack_desk(const char *position) {
        init_desk();//desk init: memory assigning, border setting
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                const char c = position[i * 8 + j];//getting figures from the line which sets default condition
                //cells init
                desk[j + 1][i + 1] = (cell_t){
                    static_cast<uchar>(get_figure_number(c)), 0, (c >= 'a' && c <= 'z'), 0, 0
                };
            }
        }
    }
    void unpack_castling(const char *position) {//default castling set 000000
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 3; ++j) {
                castling_active[i][j] = position[i * 3 + j] - '0';//default condition 000000
            }
        }
    }
    void unpack_move(const char *position) {//first move establishing
        set_current_move(position[0] - 'a' + 1, position[1] - '0',
                         position[2] - 'a' + 1, position[3] - '0',
                         get_figure_number(position[4]));
    }
    void unpack_position(const char *position) {//default positions of the desk
        unpack_desk(position);//figures establishing
        unpack_move(position + 70);//all figures and condition of castling
        unpack_castling(position + 64);
    }
    int atacks_on_king = 0;//king i snot attacked
 
    int get_mobility(int dx, int dy) {//move direction establishing
        return abs(dx) == abs(dy) ? (dx == dy ? ANGLE45 : ANGLE135) : (dx ? ANGLE0 : ANGLE90);
    }
    int is_check() {//king is attacked
        return atacks_on_king == 1;
    }
    int is_multicheck() {//if more than one figure to they attack the king simultaneously
        return atacks_on_king > 1;
    }
    int is_correct(int x, int y) {//out of desk bounds
        return x > 0 && x < 9 && y > 0 && y < 9;
    }
    int is_figure(const cell_t *cell) {
        return cell->figure != EMPTY && cell->figure != BARRIER;//is not an empty cell or a border
    }
    int is_ally(const cell_t *cell) {//if friendly figure
        return is_figure(cell) && cell->color == side;
    }
    int is_enemy(const cell_t *cell) {
        return is_figure(cell) && cell->color != side;
    }
    int is_direction_correct(const cell_t *cell, int dx, int dy) {//move direction handler
        return (cell->figure != KNIGHT && get_mobility(dx, dy) == cell->mobility) || !cell->special;
    }
    int is_moveable(int x, int y, int dx, int dy) {//move correctness
        if (!is_correct(x + dx, y + dy) || is_ally(&desk[x + dx][y + dy]) || !is_direction_correct(&desk[x][y], dx, dy))
            return 0;//if out of bound - move is impossible
        return !is_check() || desk[x + dx][y + dy].special;//if mate
    }
    void atack_handler(int x, int y, int dx, int dy) {//all possible moves generation
        if (is_correct(x + dx, y + dy)) {
            cell_t *cell = &desk[x + dx][y + dy];//moving cell pointer
            cell->attacked = 1;//cell on which figure is located can be attacked(king can get mate)
            if (is_ally(cell) && cell->figure == KING) {//if king on the cell, then attacked
                desk[x][y].special = 1;//cell from which attack goes
                ++atacks_on_king;
            }
        }
    }
    void pierce_block(int x, int y, int dx, int dy) {//blocked moves for connected move
        desk[x][y].special = 1;//figure on the cell - connected figure
        desk[x][y].mobility = get_mobility(dx, dy);//vector, on which is located connected figure
    }
    void ally_line_handler(int x, int y, int dx, int dy) {//move generation , which can be beaten for friendly figure's lines and cells under their cover
        if(!is_direction_correct(&desk[x][y], dx, dy))
            return;
        for(int i = dx, j = dy; is_correct(x + i, y + j); i += dx, j += dy) {
            if(is_moveable(x, y, i, j)) {
                add_move(x, y, x + i, y + j);//move on position
            }
            if(is_figure(&desk[x + i][y + j])) { //if you bumped into a figure
                break;
            }
        }
    }
    int pierce_detector(int x, int y, int dx, int dy, int cnt) {//blocking impossible moves
        cell_t *cell = &desk[x + dx][y + dy];//cell pointer
        if (!cnt) {//cnt = count
            atack_handler(x, y, dx, dy);//if not check - generate moves
        }
        if (cell->figure == EMPTY) {//if the cell is empty - check directions
            return cnt;//сnt = 0
        }
        ++cnt;
        cnt *= (cell->figure == KING ? -1 : 1);//if atack_handler spotted check
        if (cell->figure == BARRIER || is_enemy(cell) || cnt == 2) {//if enemy figure is bumped into a border or its own figure
            return -3;//if bumped into a border or enemy's figure
        } else { //simultaneously beat each other, check
            return cnt;
        }
    }
    void enemy_line_handler(int x, int y, int dx, int dy) {//handler of possible enemy moves
        int cnt = 0, i = 0, j = 0;
        do {//check all the cells
            i += dx;
            j += dy;;
            cnt = pierce_detector(x, y, i, j, cnt);
        } while (cnt >= 0);
        if (cnt == -1) {//while check
            atack_handler(x, y, i + dx, j + dy);
            for (i = x, j = y; desk[i][j].figure != KING; i += dx, j += dy) {//enemy king check
                desk[i][j].special = 1;//cells are connected
            }
        } else if (cnt == -2) {
            for (i = x, j = y; !is_ally(&desk[i][j]); i += dx, j += dy);
            pierce_block(i, j, dx, dy);
        }
    }
 
    void X_handler(int x, int y, int vh, int d, line_handler_t handl) {//direction handler
        if (vh) {//if vertical or horizontal
            (this->*handl)(x, y, 1, 0);//right
            (this->*handl)(x, y, -1, 0);//left
            (this->*handl)(x, y, 0, 1);//up
            (this->*handl)(x, y, 0, -1);//down
        }
        if (d) {//if move is horizontal
            (this->*handl)(x, y, 1, 1);//up-right
            (this->*handl)(x, y, 1, -1);//down-right
            (this->*handl)(x, y, -1, 1);//left-up
            (this->*handl)(x, y, -1, -1);//left-down
        }
    }
    void ally_knight_handler(int x, int y) {//move generator for a friendly king
        for(int i = 1; i <= 2; ++i) {//two lines move by x axis
            int j = 3 - i;//position on y axis
            if(is_moveable(x, y, i, j)) {//if the move is possible, then do it
                add_move(x, y, x + i, y + j);//up and right
            }
            if(is_moveable(x, y, i, -j)) {
                add_move(x, y, x + i, y - j);//right and down
            }
            if(is_moveable(x, y, -i, j)) {
                add_move(x, y, x - i, y + j);//left and up
            }
            if(is_moveable(x, y, -i, -j)) {
                add_move(x, y, x - i, y - j);//left and down
            }
        }
    }
    void enemy_knight_handler(int x, int y) {//enemy king
        for (int i = 1; i <= 2; ++i) {//проход по 2 рядам, координата по х // 2 rows move, x coordinates
            int j = 3 - i;//y coordinates
            atack_handler(x, y, i, j);
            atack_handler(x, y, i, -j);
            atack_handler(x, y, -i, j);
            atack_handler(x, y, -i, -j);
        }
    }
    void ally_king_handler(int x, int y) {//move generator for friendly king
        for (int i = -1; i <= 1; ++i) {//in all directions horizontally
            for (int j = -1; j <= 1; ++j) {//in all directions vertically
                if (is_correct(x + i, y + j) && !desk[x + i][y + j].attacked &&
                        !is_ally(&desk[x + i][y + j])) {
                    add_move(x, y, x + i, y + j);
                }
            }
        }
    }
    void enemy_king_handler(int x, int y) {//possible moves generation for an enemy king
        for (int i = -1; i <= 1; ++i) {//in all the directions horizontally
            for (int j = -1; j <= 1; ++j) {//in all the directions vertically
                if (desk[x + i][y + j].figure != KING) {
                    atack_handler(x, y, i, j);
                }
            }
        }
    }
    void pawn_kill_move(int x, int y, int dx, int dy) {//move generator, using which a pawn can beat enemie's figure: f2->g3, f2->e3
        if ((is_enemy(&desk[x + dx][y + dy]) && is_moveable(x, y, dx, dy))
                || (extra.exist && x + dx == extra.X && y + dy == extra.Y
                    && (is_moveable(x, y, dx, dy) || is_moveable(x, y, dx, 0)))) {
            add_move(x, y, x + dx, y + dy);
        }
    }
    void ally_pawn_handler(int x, int y) {//friendly pawn move generation
        int t = (side ? 1 : -1);
        //side check, t = 1 for white, -1 for black, white go upwards, black downwards
        pawn_kill_move(x, y, 1, t);
        pawn_kill_move(x, y, -1, t);
 
        if (is_figure(&desk[x][y + t])) {//impossible to move on the cell, where figure exists
            return;
        }
        if (is_moveable(x, y, 0, t)) {//1 cell move generator
            add_move(x, y, x, y + t);
        }
        //2 cells move generation
        if (y == 7 - 5 * side && !is_figure(&desk[x][y + 2 * t]) && is_moveable(x, y, 0, 2 * t)) {
            add_move(x, y, x, y + 2 * t);
        }
    }
    void enemy_pawn_handler(int x, int y) {//pawn possible moves
        //side = -1 for black, for white 1, если white move, downwards move generation, for black vice versa
        atack_handler(x, y, 1, -(side ? 1 : -1));//right side move generator
        atack_handler(x, y, -1, -(side ? 1 : -1));//left side move generator 
    }
    void ally_rook_handler(int x, int y) {//friendly rook check
        X_handler(x, y, 1, 0, &__Chess::ally_line_handler);//пhorazintal and vercical check
    }
    void enemy_rook_handler(int x, int y) {//enemy rook check
        X_handler(x, y, 1, 0, &__Chess::enemy_line_handler);//horizontal and vertical check
    }
    void ally_bishop_handler(int x, int y) {//friendly bishop check
        X_handler(x, y, 0, 1, &__Chess::ally_line_handler);//diagonal check
    }
    void enemy_bishop_handler(int x, int y) {//enemy bishop check
        X_handler(x, y, 0, 1, &__Chess::enemy_line_handler);//diagonal check
    }
 
    void ally_queen_handler(int x, int y) {//friendly queen check
        X_handler(x, y, 1, 1, &__Chess::ally_line_handler);//vertical/horizontal and diagonal check
    }
    void enemy_queen_handler(int x, int y) {//enemy queen check
        X_handler(x, y, 1, 1, &__Chess::enemy_line_handler);//vertical/horizontal and diagonal check
    }
 
    void ally_figure_handler(int x, int y) {//figure search and move generator establishing
        const static cell_handler_t handlers[] = {//array with move generation functions
            &__Chess::ally_pawn_handler, &__Chess::ally_queen_handler,
            &__Chess::ally_king_handler, &__Chess::ally_knight_handler,
            &__Chess::ally_bishop_handler, &__Chess::ally_rook_handler
        };
        if(!is_figure(&desk[x][y]) || is_enemy(&desk[x][y])
                || (is_multicheck() && desk[x][y].figure != KING)) {//
            return;//if it is not a friendly figure or enemy's or when mate is done by more than 1 figure
        }
        (this->*handlers[desk[x][y].figure - 1])(x, y);
    }
    void enemy_figure_handler(int x, int y) {
        const static cell_handler_t handlers[] = {
            &__Chess::enemy_pawn_handler, &__Chess::enemy_queen_handler,
            &__Chess::enemy_king_handler, &__Chess::enemy_knight_handler,
            &__Chess::enemy_bishop_handler, &__Chess::enemy_rook_handler
        };
        //if the figure is not friendly for enemy's or an empty cell
        if (!is_figure(&desk[x][y]) || is_ally(&desk[x][y])) {
            return;
        }
        (this->*handlers[desk[x][y].figure - 1])(x, y);
    }
    void desk_handler(cell_handler_t handl) {
        for (int i = 1; i <= 8; ++i) {
            for (int j = 1; j <= 8; ++j) {
                (this->*handl)(i, j);
            }
        }
    }
    void castle_handler() {
        if (is_check() || is_multicheck() || castling_active[side][1]) {//
            return;//if check
        }
        int sideline = 8 - side * 7;//1st or 8th line
        //if the king is on the cell on which he can move
        //the king is not attacked
        if (!castling_active[side][0]
                && !desk[4][sideline].attacked && !is_figure(&desk[4][sideline])
                && !desk[3][sideline].attacked && !is_figure(&desk[3][sideline])) {
            add_move(5, sideline, 3, sideline);//move e1->c1, e8->c8
        }
        //if the cell on which king is moving is not attacked
        if (!castling_active[side][2] && !desk[6][sideline].attacked && !is_figure(&desk[6][sideline])
                && !desk[7][sideline].attacked && !is_figure(&desk[7][sideline])) {
            add_move(5, sideline, 7, sideline);//move e1->g1, e8->g8
        }
    }
 
    void position_handler() {
        atacks_on_king = 0;
        clear_move_table();
        clear_desk_state();
        release_current_move();
        desk_handler(&__Chess::enemy_figure_handler);
        desk_handler(&__Chess::ally_figure_handler);
        castle_handler();
    }
    void UpdateMoves() {
        moves.clear();
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < moveptr_list[i]; j += 4) {
                moves.push_back(make_tuple(
                                    move_table[i][j] - 'a' + 1,
                                    move_table[i][j + 1] - '0',
                                    move_table[i][j + 2] - 'a' + 1,
                                    move_table[i][j + 3] - '0'
                                ));
            }
        }
        destroy_move_table();
        init_move_table();
    }
    void output_desk() {
        cout << "\n\t\t\t\t         Chess game\n\n";
        cout << "\t\t\t  ----------------------------------------" << endl;
        for (int i = 8; i >= 1; --i) {
            cout << "\t\t\t" << i << " | ";
            for (int j = 1; j <= 8; ++j) {
                putchar(get_figure_symbol(desk[j][i].figure) +
                        (desk[j][i].color || desk[j][i].figure == EMPTY ? 0 : 'A' - 'a'));
                if (j < 8)
                    cout << " || ";
            }
            cout << " |\n\t\t\t  ----------------------------------------\n";
        }
        cout << " \t\t\t    a    b    c    d    e    f    g    h   \n\n";
    }
  public:
    clock_t start;
    explicit __Chess() {
        init_move_table();
        unpack_position(starting);
        position_handler();
        UpdateMoves();
    }
    void ChessAgain() {
        white_moves = black_moves = 0;
        init_move_table();
        unpack_position(starting);
        position_handler();
        UpdateMoves();
    }
    void start_timer() {
        start = clock();
    }
 
    void GameInfo() {
        double time = (clock() - start) / CLOCKS_PER_SEC / 60.;
        cout << "\t\t\tTime of the game: " << (int)time << " minutes, " << setprecision(2)<< (time - (int)time)*100 << " seconds\n";
        cout << "\t\t\tTotal moves: " << white_moves + black_moves << endl;
        cout << "\t\t\tWhite_moves -- " << white_moves << ", black_moves -- " << black_moves << endl;
    }
    bool Turn(int number) {
        if (number < 0 || number >= moves.size()) {
            return false;
        }
        tie(current_move.fromX, current_move.fromY, current_move.toX, current_move.toY) = moves[number];
        position_handler();//new desk generator
        UpdateMoves();
        return true;
    }
    void Piz(){
        ChessAgain();
        return Play();
    }
    void Play() {
        start_timer();
        while (!moves.empty()) {
            output_desk();
            sort(moves.begin(), moves.end());
            cout << setw(52) << (side ? "White's turn\n" : "Black's turn\n");
            cout << setw(54) << "Possible moves\n\n";
            for (int i = 0; i < moves.size(); ++i) {
                if (i % 2 == 0) cout << "\t\t\t\t";
                cout << setw(2) << i + 1 << ". " << char(get<0>(moves[i]) + 96) << get<1>(moves[i]) << "->"
                     << char(get<2>(moves[i]) + 96) << get<3>(moves[i]) << "   ";
                if(i % 2 != 0)  cout << endl;
            }
            cout << endl;
            int number;
            cin >> number;//ввод номеры хода
            Turn(number - 1);
        }
        GameInfo();
    }
 
    void StartGame() {
        bool indicator;
        string answer;
            cout << "\n\t\t\t\tWelcome to Chess game!\n\n";
            cout << setw(57) << "Choose option and enter:\n";
            cout << setw(61) << "\t\t<play>, <rules>, <exit>, <again>\n\t\t\t\t\t";
            cin >> answer;
            if (answer == "play") {
                system("clear");
                Play();
            } else if(answer == "rules") {
                system("clear");
                indicator = true;
            } else if (answer == "again") {
                system("clear");
                ChessAgain();
                //return StartGame();
                Play();
            } else if (answer == "exit") {
                exit(0);
                system("clear");
                GameInfo();
                cout << "\n\n\t\t\t   Thank you for game! Hope to see you later.\n";
                indicator = false;
            } else {
                cout << "\n\n\t\t\t\tYour answer is incorrect.\n";
                indicator = true;
            }
    }
 
};
 
struct Chess {
 
    void StartGame() {
        while (true) {
            __Chess game = __Chess();
            game.StartGame();
        }
    }
 
};
 
