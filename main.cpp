#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
using namespace std;

const int BOARD_SIZE = 8;
const int squareSize = 100;
const float pieceScale = 0.3f;   // normal piece
const float kingScale = 0.9f;  // kings rendered slightly larger

// Piece codes
// 0 = empty
// 1 = black piece
// 2 = white piece
// 3 = black king
// 4 = white king

// Directions for normal moves
const int blackDirs[2][2] = { {1,-1}, {1,1} };  // black moves down
const int whiteDirs[2][2] = { {-1,-1}, {-1,1} }; // white moves up
const int kingDirs[4][2] = { {1,-1}, {1,1}, {-1,-1}, {-1,1} };

inline bool isInside(int r, int c) { return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE; }
inline bool isPlayerPiece(int p, int pl) { return (pl == 1) ? (p == 1 || p == 3) : (p == 2 || p == 4); }
inline bool isKing(int p) { return p == 3 || p == 4; }
inline int opponent(int p) { return 3 - p; }

bool validSimpleMove(int board[8][8], int player, int fr, int fc, int tr, int tc) {
    if (!isInside(tr, tc) || board[tr][tc] != 0) return false;
    int piece = board[fr][fc];
    if (isKing(piece)) {
        for (int i = 0; i < 4; i++) if (fr + kingDirs[i][0] == tr && fc + kingDirs[i][1] == tc) return true;
    }
    else {
        const int(*d)[2] = (player == 1) ? blackDirs : whiteDirs;
        for (int i = 0; i < 2; i++) if (fr + d[i][0] == tr && fc + d[i][1] == tc) return true;
    }
    return false;
}

void validCaptureMove(int board[8][8], int player, int fr, int fc, int tr, int tc, int& mr, int& mc) {
    mr = mc = -1;
    if (!isInside(tr, tc) || board[tr][tc] != 0) return;
    int piece = board[fr][fc];
    if (isKing(piece)) {
        for (int i = 0; i < 4; i++) {
            int midR = fr + kingDirs[i][0], midC = fc + kingDirs[i][1];
            int rr = fr + 2 * kingDirs[i][0], cc = fc + 2 * kingDirs[i][1];
            if (rr == tr && cc == tc && isPlayerPiece(board[midR][midC], opponent(player))) { mr = midR; mc = midC; return; }
        }
    }
    else {
        const int(*d)[2] = (player == 1) ? blackDirs : whiteDirs;
        for (int i = 0; i < 2; i++) {
            int midR = fr + d[i][0], midC = fc + d[i][1];
            int rr = fr + 2 * d[i][0], cc = fc + 2 * d[i][1];
            if (rr == tr && cc == tc && isPlayerPiece(board[midR][midC], opponent(player))) { mr = midR; mc = midC; return; }
        }
    }
}

bool hasAnyCapture(int board[8][8], int player) {
    for (int r = 0; r < 8; r++)for (int c = 0; c < 8; c++) if (isPlayerPiece(board[r][c], player)) {
        int piece = board[r][c];
        const int(*d)[2] = isKing(piece) ? kingDirs : ((player == 1) ? blackDirs : whiteDirs);
        int lim = isKing(piece) ? 4 : 2;
        for (int i = 0; i < lim; i++) {
            int mr = r + d[i][0], mc = c + d[i][1];
            int tr = r + 2 * d[i][0], tc = c + 2 * d[i][1];
            if (isInside(tr, tc) && board[tr][tc] == 0 && isPlayerPiece(board[mr][mc], opponent(player))) return true;
        }
    }
    return false;
}

bool hasAnyMoves(int board[8][8], int player) {
    if (hasAnyCapture(board, player)) return true;
    for (int r = 0; r < 8; r++)for (int c = 0; c < 8; c++) if (isPlayerPiece(board[r][c], player)) {
        int piece = board[r][c];
        const int(*d)[2] = isKing(piece) ? kingDirs : ((player == 1) ? blackDirs : whiteDirs);
        int lim = isKing(piece) ? 4 : 2;
        for (int i = 0; i < lim; i++) {
            int tr = r + d[i][0], tc = c + d[i][1];
            if (isInside(tr, tc) && board[tr][tc] == 0) return true;
        }
    }
    return false;
}

int countPieces(int board[8][8], int player) { int cnt = 0; for (int r = 0; r < 8; r++)for (int c = 0; c < 8; c++)if (isPlayerPiece(board[r][c], player))cnt++; return cnt; }

void tryPromote(int board[8][8], int row, int col) { if (board[row][col] == 1 && row == 7)board[row][col] = 3; else if (board[row][col] == 2 && row == 0)board[row][col] = 4; }

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 800), "Checkers with Rules");

    sf::Texture texBlack, texWhite, texBlackKing, texWhiteKing;
    sf::Font font;
    if (!texBlack.loadFromFile("Assets/black.png") ||
        !texWhite.loadFromFile("Assets/white.png") ||
        !texBlackKing.loadFromFile("Assets/blackking.png") ||
        !texWhiteKing.loadFromFile("Assets/whiteking.png") ||
        !font.loadFromFile("Assets/arial.ttf")) {
        cout << "Failed to load image/font resources.\n"; return -1;
    }

    sf::SoundBuffer moveBuf, capBuf;
    if (!moveBuf.loadFromFile("Assets/move.wav") ||
        !capBuf.loadFromFile("Assets/capture.wav")) {
        cout << "Failed to load sound resources.\n"; return -1;
    }
    sf::Sound sndMove(moveBuf), sndCap(capBuf);
    sndMove.setVolume(60);
    sndCap.setVolume(80);

    int board[8][8] = { 0 };
    for (int r = 0; r < 8; r++)for (int c = 0; c < 8; c++) if ((r + c) % 2) { board[r][c] = (r < 3 ? 1 : (r > 4 ? 2 : 0)); }

    int currentPlayer = 1, selR = -1, selC = -1, winner = 0;
    bool mustChain = false, gameOver = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)window.close();

            if (!gameOver && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                int col = event.mouseButton.x / squareSize, row = event.mouseButton.y / squareSize;
                if ((row + col) % 2 && isInside(row, col)) {
                    if (selR == -1) {
                        if (isPlayerPiece(board[row][col], currentPlayer)) {
                            selR = row; selC = col;
                        }
                    }
                    else {
                        if (board[row][col] == 0) {
                            int midR = -1, midC = -1;  // initialize here
                            bool ok = false;
                            bool mustCap = hasAnyCapture(board, currentPlayer);

                            if (mustCap) {
                                validCaptureMove(board, currentPlayer, selR, selC, row, col, midR, midC);
                                ok = (midR != -1 && midC != -1);
                            }
                            else {
                                ok = validSimpleMove(board, currentPlayer, selR, selC, row, col);
                            }

                            if (ok) {
                                board[row][col] = board[selR][selC];
                                board[selR][selC] = 0;

                                if (midR != -1 && midC != -1) {
                                    board[midR][midC] = 0;
                                    sndCap.stop(); sndCap.play();
                                }
                                else {
                                    sndMove.stop(); sndMove.play();
                                }

                                tryPromote(board, row, col);

                                if (midR != -1 && midC != -1) {
                                    selR = row; selC = col;

                                    // Check if the selected piece can still capture
                                    bool pieceCanCapture = false;
                                    int piece = board[selR][selC];
                                    const int(*d)[2] = isKing(piece) ? kingDirs : ((currentPlayer == 1) ? blackDirs : whiteDirs);
                                    int limit = isKing(piece) ? 4 : 2;

                                    for (int i = 0; i < limit; i++) {
                                        int mr = selR + d[i][0];
                                        int mc = selC + d[i][1];
                                        int tr = selR + 2 * d[i][0];
                                        int tc = selC + 2 * d[i][1];

                                        if (isInside(tr, tc) && board[tr][tc] == 0 && isPlayerPiece(board[mr][mc], opponent(currentPlayer))) {
                                            pieceCanCapture = true;
                                            break;
                                        }
                                    }

                                    if (pieceCanCapture) {
                                        mustChain = true;
                                    }
                                    else {
                                        mustChain = false;
                                        currentPlayer = opponent(currentPlayer);
                                        selR = selC = -1;
                                    }
                                }
                                else {
                                    mustChain = false;
                                    currentPlayer = opponent(currentPlayer);
                                    selR = selC = -1;
                                }


                                int p1 = countPieces(board, 1), p2 = countPieces(board, 2);
                                if (p1 == 0 || !hasAnyMoves(board, 1)) { gameOver = true; winner = 2; }
                                else if (p2 == 0 || !hasAnyMoves(board, 2)) { gameOver = true; winner = 1; }
                            }
                        }
                        else if (!mustChain && isPlayerPiece(board[row][col], currentPlayer)) {
                            selR = row; selC = col;
                        }
                    }
                }
            }
        }

        window.clear();
        for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
            sf::RectangleShape sq(sf::Vector2f(squareSize, squareSize));
            sq.setPosition(c * squareSize, r * squareSize);
            sq.setFillColor((r + c) % 2 ? sf::Color(245, 245, 220) : sf::Color(139, 69, 19));


            window.draw(sq);

            if (r == selR && c == selC) {
                sf::RectangleShape h(sf::Vector2f(squareSize, squareSize));
                h.setPosition(c * squareSize, r * squareSize);
                h.setFillColor(sf::Color::Transparent);
                h.setOutlineThickness(4);
                h.setOutlineColor(sf::Color::Green);
                window.draw(h);
            }

            if (board[r][c]) {
                sf::Sprite sp;
                switch (board[r][c]) {
                case 1: sp.setTexture(texBlack); break;
                case 2: sp.setTexture(texWhite); break;
                case 3: sp.setTexture(texBlackKing); break;
                case 4: sp.setTexture(texWhiteKing); break;
                }
                float scale = isKing(board[r][c]) ? kingScale : pieceScale;
                sp.setScale(scale, scale);
                sf::FloatRect b = sp.getLocalBounds();
                sp.setOrigin(b.width / 2, b.height / 2);
                sp.setPosition(c * squareSize + squareSize / 2.f, r * squareSize + squareSize / 2.f);
                window.draw(sp);
            }
        }

        if (gameOver) {
            sf::Text t;
            t.setFont(font);
            t.setString("Player " + to_string(winner) + " wins!");
            t.setCharacterSize(50);
            t.setFillColor(sf::Color::White);
            t.setStyle(sf::Text::Bold);
            sf::FloatRect br = t.getLocalBounds();
            t.setOrigin(br.width / 2, br.height / 2);
            t.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f);

            sf::RectangleShape ov(sf::Vector2f(window.getSize().x, window.getSize().y));
            ov.setFillColor(sf::Color(0, 0, 0, 200));
            window.draw(ov);
            window.draw(t);
        }

        window.display();
    }
    return 0;
}
