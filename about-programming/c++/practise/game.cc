#include "local.h"

struct base {
    string name;
    int posx;
    int posy;
    base(string s, int x, int y): name(s), posx(x), posy(y) {}
};

struct enemy : base {
    enemy(string s, int x, int y): base(s, x, y){}
};

struct trap : base {
    trap(string s, int x, int y): base(s, x, y){}
};
    
struct treasure: base {
    treasure(string s, int x, int y): base(s, x, y){}
};

class Pad;
struct Player: base {
    Player(string s, int x, int y, Pad p): base(s, x, y), pad(p){}
}

class Pad {
    private:
        vector<vector<int>> vii;
        int width;
        int height;
        vector<enemy> ve;
        vector<trap> vt;
        random_device rd;
        default_random_engine eng;
        uniform_int_distribution<> uw;
        uniform_int_distribution<> uh;
        uniform_int_distribution<> ue;
        
    public:
        Pad(int e = 0, int t = 0, int w = 8, int h = 8):
                width(w), height(h), eng(rd()), uw(0, width-1), uh(0, height-1), ue(0, 3){

            // generate enemy, trap, currently can overlap
            for (int i = 0; i < e; ++i) ve.push_back({"enemy", uh(eng), uw(eng)});
            for (int i = 0; i < t; ++i) vt.push_back({"trap", uh(eng), uw(eng)});
            vii.reserve(h);
            for (int i = 0; i < h; ++i) {
                vii[i].reserve(w);
            }
            for (int i = 0; i < h; ++i) {
                for (int j = 0; j < w; ++j) {
                    vii[i].push_back(0);
                }
            }
            for (auto e : ve) { //enemy 1
                cout << e.posx << " " << e.posy << endl;
                vii[e.posx][e.posy] = 1;
            }
            for (auto t : vt) { // trap 2
                cout << t.posx << " " << t.posy << endl;
                vii[t.posx][t.posy] = 2;
            }
        }

        // update enemy's position
        void update()  {
            for (auto& e : ve) {
                int x, y;
                switch (ue(eng)) {
                case 0: // up
                    x = max(e.posx-1, 0);
                    y = e.posy;
                    break;
                case 1: // down
                    x = min(e.posx+1, height-1);
                    y = e.posy;
                    break;
                case 2: // left
                    x = e.posx;
                    y = max(e.posy-1, 0);
                    break;
                case 3: //right
                    x = e.posx;
                    y = min(e.posy+1, width-1);
                    break;
                default:
                    cout << "error occurred" << endl;
                    exit(0);
                    break;
                }
                vii[e.posx][e.posy] = 0;
                vii[x][y] = 1;
                e.posx = x;
                e.posy = y;
            }
        }

        void printPad() {
            cout << endl;
            for (int i = 0; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    cout << " | " << vii[i][j] ;
                }
                cout << " | " << endl;
            }
        }
};


int main() {
    Pad pad(2,0,8,8);
    pad.printPad();
    sleep(1);
    while (true) {
        pad.update();
        pad.printPad();
        usleep(500000);
    }
    /*
    Pad pad{2,2}; // two trap and two enemy
    Player player{pad};
    int input;
    while (cin >> input) {
        switch (input) {
            case 'w' : player.up(); break;
            case 's' : player.down(); break;
            case 'a' : player.left(); break;
            case 'd' : player.right(); break;
            default: cout << "can not recognize key " << input << endl; break;
        }
        player.updateMap();
        player.showMap();
        if (player.dead()) {
            cout << "dead" << endl;
            break;
        }
        if (player.win()) {
            cout << "win" << endl;
            break;
        }
    }
    */
    return 0;
}
