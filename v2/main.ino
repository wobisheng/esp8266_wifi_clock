#include <LedControl.h>
#include <ArduinoQueue.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Metro.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#define WIFI_CONFIG 0
#define AMAP_CONFIG 100
#define AP_CONFIG 200
#define WORD_CONFIG 300
#define MAX 8
ESP8266WebServer server( 80 );

LedControl lc = LedControl( 12, 14, 15, 4 );

WiFiUDP ntpUDP;
NTPClient TimeClient( ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000 );
Metro  game_tick   = Metro( 500 );//游戏时间周期
Metro  time_tick   = Metro( 60000 );//时间更新周期60s
Metro  weather_tick   = Metro( 3600000 );//天气检查周期1h

byte game_mode = 0;//0号屏模式

struct snake_Node
    {
      int x;
      int y;
    };
ArduinoQueue<snake_Node> body(65);

class snake_game//蛇类
{
  private:
    bool snake_eat_s = false;
    int snake_length = 0,snake_food_point[2] = {5,5};
    int snake[65][2] = {0};
    void Gen_map()
    {
      int n = body.item_count();
      snake_length = n;
      for (int i = n - 1 ; i >= 0 ; i --)
      {
      snake[i][0] = body.getHead().x;
      snake[i][1] = body.getHead().y;
      body.dequeue();
      }
    }
    
    void Draw(int *x,int *y,int Len)
    {
      byte Map[8]={ 0 };
      for (int i = 0; i < Len; i ++)
      {
        Map[7-y[i]] = Map[7-y[i]] + pow(2,7-x[i]);
      }
      for (int i = 0; i <=7; i ++)
      {
       if (snake_form == 0) {lc. setRow(0,i,Map[7-i]);}
       else if(snake_form == 255) {lc. setRow(0,i,snake_form - Map[7-i]);}
       else {lc. setRow(0,i,Map[7-i]^B00001111);}
      }
      delay(200);
    }
    
    void Gen_point()
    {
      randomSeed(millis());
      int food_point_temp_x = random(0, 8);
      int food_point_temp_y = random(0, 8);
      bool Status = false;
      while(!Status)
      {
      for (int i =0; i < snake_length; i ++)
      {
        if (snake[i][0] == food_point_temp_x && snake[i][1] == food_point_temp_y)
        {
          food_point_temp_x = random(0, 8);
          food_point_temp_y = random(0, 8);
          i = 0;
        }
        if (i == snake_length - 1)
        {
          Status = true;
          break;
        }
      }
      }
      snake_food_point[0] = food_point_temp_x;
      snake_food_point[1] = food_point_temp_y;
    }
    
    void bound_judge()
    {
      bool Status = true;
      for (int i =0; i < snake_length; i ++)
      {
      if (!snake_map_infinity)
      {
        if (snake[i][0] < 0 || snake[i][0] > 7 || snake[i][1] > 7||snake[i][1] < 0) {Status = false;}
      }
      else
      {
        if (snake[i][0] < 0) {snake[i][0] += 8;}
        if (snake[i][1] < 0) {snake[i][1] += 8;}
        if (snake[i][0] > 7) {snake[i][0] -= 8;}
        if (snake[i][1] > 7) {snake[i][1] -= 8;}
      }
      if (i>=1&&(snake[0][1] == snake[i][1])&&(snake[0][0] == snake[i][0])) {Status = false;}
      if (snake[0][0] == snake[1][0]&&snake[0][1] == snake[1][1]) {Status = false;}
      }
      if (Status)
      Convert();
      else 
      {
      lc.clearDisplay(0);
      lc. setColumn(0,4,B11111111);
      while(!body.isEmpty())
      {
        body.dequeue();
      }
      snake_Node first = {0,0};
      body.enqueue(first);
      snake_Direction = 'd';
      snake_food_point[0] = 5;
      snake_food_point[1] = 5;
      delay(50);
      Move(snake_Direction);
      }
    }

    void food_judge(int x,int y)
    {
        if (x ==snake_food_point[0]&&y == snake_food_point[1])
        {
        snake_eat_s = true;
        }
        else
        {
        body.dequeue();
        }
    }

    void Convert()
    {
      int temp_x[snake_length + 1];
      int temp_y[snake_length + 1];
      for (int i = 0; i < snake_length ; i ++)
      {
      temp_x[i] = snake[i][0];
      temp_y[i] = snake[i][1];
      }
      temp_x[snake_length] = snake_food_point[0];
      temp_y[snake_length] = snake_food_point[1];
      Draw(temp_x,temp_y,snake_length + 1);
      snake_Node temp;
      for (int i = snake_length - 1; i >= 0; i --)
      {
      temp.x = snake[i][0];
      temp.y = snake[i][1];
      body.enqueue(temp);
      }
      for (int i = snake_length - 1; i >= 0; i --)
      {
      snake[i][0] = 0;
      snake[i][1] = 0;
      }
    }
    void Move(char receive)
    {
        snake_Node temp;
        temp.x = body.getTail().x;
        temp.y = body.getTail().y;
        switch (receive)
        {
         case 'u':
          temp.y --;
          body.enqueue(temp);
          break;
         case 'd':
          temp.y ++;
          body.enqueue(temp);
          break;
         case 'r':
          temp.x ++ ;
          body.enqueue(temp);
          break;
         case 'l':
          temp.x --;
          body.enqueue(temp);
          break;
        };
        food_judge(temp.x,temp.y);
        Gen_map();
        if (snake_eat_s){Gen_point();snake_eat_s = false;}
        bound_judge();
    }
    public:
    bool snake_map_infinity = false;
    int snake_form = 0;
    char snake_Direction = 'd';
    void AutoMove()
    {
      Move(snake_Direction);
    }
    void Start()
    {
      while(!body.isEmpty())
      {
        body.dequeue();
      }
      snake_Node first = {0,0};
      body.enqueue(first);
      snake_Direction = 'd';
      snake_food_point[0] = 5;
      snake_food_point[1] = 5;
    }
};

class tetris_game//俄罗斯方块类
{
  private:
    bool tetris_Map[8][8] = { 0 };
    int shape_map[7][3][2] = {
                {{0,-1},{1,-1},{1,0}},//方块
                {{0,-1},{-1,0},{1,0}},//T形
                {{0,-2},{0,-1},{1,0}},//L形
                {{0,-2},{0,-1},{0,1}},//|形
                {{-1,0},{0,-1},{1,-1}},//Z形
                {{0,-2},{0,-1},{-1,0}},//对称L
                {{-1,-1},{0,-1},{1,0}}//对称Z
                }; 
    int tetris_store_x[4] = { 0 };
    int tetris_store_y[4] = { 0 };

    bool judge_bound(int * x,int *y)
    {
      for (int i =0 ; i < 4 ; i ++)
      {
      if (tetris_store_y[i]<0||y[i]<0) return true;
      if ( x[i] < 0|| x[i] > 7 || y[i] > 7)
      { return false; }
      }
      for (int i = 0; i < 4; i ++)
      {
        tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 0; 
      }
      for (int i = 0; i < 4; i ++)
      {
      if(tetris_Map[y[i]][x[i]] == 1)
      {
        for (int i = 0; i < 4; i ++)
          tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 1;
        return false;
      }
      }
      for (int i = 0; i < 4; i ++)
        tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 1;
      return true;
    }

    bool judge_bound(int * coordinate)
    {
      for (int i = 0; i < 4 ; i ++)
      {
      if (coordinate[i]>7||coordinate[i]<0) return false;
      }
      for (int i = 0; i < 4 ; i ++)
      {
      if (tetris_store_y[i]<0) return true;
      }
      for (int i = 0; i < 4 ; i ++)
      {
      tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 0;
      }
      for (int i = 0; i < 4 ; i ++)
      {
      if(tetris_Map[tetris_store_y[i]][coordinate[i]]) 
      {
       for (int i = 0; i < 4 ; i ++)
        {
        tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 1;
        }
       return false; 
      }
      }
      for (int i = 0; i < 4 ; i ++)
      {
      tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 1;
      }
      return true;
    }

    bool judge_finish()
    {
      server.handleClient();
      delay(100);
      for (int i = 0; i < 4 ; i ++)
      {
      if (tetris_store_y[i] == 7)
      {return true;}
      }
      for (int i = 0; i < 4 ; i ++)
      if (tetris_store_y[i]>=0)
        { tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 0; }
      for (int i = 0; i < 4; i ++ )
      {
      if (tetris_store_y[i]>=0&&tetris_Map[tetris_store_y[i]+1][tetris_store_x[i]])
      {
        for (int i = 0; i < 4; i ++ )
        if (tetris_store_y[i]>=0)
          tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 1;
        return true; 
      }
      }
      return false;
    }

    void summon()
    {
      unsigned int shape = random(0,7);
      tetris_store_x[0] =  3;
      tetris_store_y[0] =  -1;
      tetris_store_x[1] =  3 + shape_map[shape][0][0];
      tetris_store_y[1] =  -1 + shape_map[shape][0][1];
      tetris_store_x[2] =  3 + shape_map[shape][1][0];
      tetris_store_y[2] =  -1 + shape_map[shape][1][1];
      tetris_store_x[3] =  3 + shape_map[shape][2][0];
      tetris_store_y[3] =  -1 + shape_map[shape][2][1];
      game_over();
      Clear();
    }

    void Clear()
    {
      int i = 7;
      while (i >= 1)
      {
      for (int j = 0; j <= 7; j ++)
      {
        if (!tetris_Map[i][j])
        {
        i --;
        break;
        }
        else if (j == 7)
        {
        for (int m = i; m > 0; m --)
        {
          for (int n = 0; n <= 7; n ++)
           tetris_Map[m][n] = tetris_Map[m-1][n];
        }
        }
      }
      }
    }

    void game_over()
    {
      for (int i = 0; i <=7 ;i ++)
      {
      if (tetris_Map[1][i] == 1) 
      {
        delay(500);
         lc.clearDisplay(0);
         lc. setColumn(0,4,B11111111);
         Start();
         delay(50);
      }
      }
    }

    void Move()
    {
      tetris_Position = false;
      int temp_y[4];
      int temp_x[4];
      for (int i = 0; i < 4; i ++)
      temp_x[i] = tetris_store_x[i];
      for (int i = 0; i < 4; i ++)
      {
      temp_y[i] = tetris_store_y[i] + 1;
      }
      Convert(temp_x,temp_y);
      tetris_Position = true;
      server.handleClient();
      delay(300);
      if(judge_finish())
      {
      summon();
      }
    }

    void Convert(int * new_x,int *new_y)
    {
        for (int i = 0; i < 4; i ++)
        {
        if (tetris_store_y[i] >= 0)
        tetris_Map[tetris_store_y[i]][tetris_store_x[i]] = 0;
        }
        for (int i =0; i < 4; i ++)
        {
        if (new_y[i]>= 0)
        tetris_Map[new_y[i]][new_x[i]] = 1;
        }
        for (int i =0; i < 4; i ++)
        {
        tetris_store_x[i] = new_x[i];
        tetris_store_y[i] = new_y[i];
        }
      Draw();
    }

    void Draw()
    {
      for (int j = 0; j <=7; j ++)
      {
      int temp = 0;
      for (int i = 0; i <= 7; i ++)
      {
        if (tetris_Map[j][i]) {temp += pow(2,7-i);}
        if (i == 7) {lc. setRow(0,j,temp);}
      }
      }
    }
  public:
    bool tetris_Moving = true;
    bool tetris_Position = false;
    void AutoMove()
    {
      if (tetris_Moving)
      Move();
    }
    void Start()
    {
      game_mode = 3;
      for (int i = 0; i <= 7 ; i ++)
      for (int j = 0; j <= 7; j ++)
        tetris_Map[i][j] = 0;
      summon();
    }
    void Rotate()
    {
      int temp_x[4],temp_y[4];
      temp_x[0] = tetris_store_x[0];
      temp_y[0] = tetris_store_y[0];
      for (int i = 1; i < 4; i ++)
      {
      temp_x[i] = tetris_store_x[i];
      temp_y[i] = tetris_store_y[i];
      }
      for (int i = 1; i < 4; i ++)
      {
        int minus_x = temp_x[i] - temp_x[0];
        int minus_y = temp_y[i] - temp_y[0];
        if (minus_x == 0)
        { minus_x = - minus_y; minus_y = 0;}
        else if (minus_y == 0)
        { minus_y = minus_x; minus_x = 0;}
         else if (minus_x > 0&& minus_y > 0)
         { minus_x = - minus_x; }
         else if (minus_x < 0 && minus_y > 0)
         {minus_y = - minus_y;}
         else if (minus_x< 0 && minus_y < 0)
         {minus_x = - minus_x;}
         else if (minus_x > 0 && minus_y < 0)
         {minus_y = - minus_y;}
        temp_x[i] = temp_x[0] + minus_x;
        temp_y[i] = temp_y[0] + minus_y;
       }
       if (judge_bound(temp_x,temp_y))
       {
        Convert(temp_x,temp_y);
        if(judge_finish())
        {
        summon();
        }
       }
    }
    void right()
    {
      int temp_x[4],temp_y[4];
      for (int i = 0; i < 4; i ++)
      temp_y[i] = tetris_store_y[i];
      for (int i = 0; i < 4; i ++)
      {
      temp_x[i] = tetris_store_x[i] + 1;
      }
      if(judge_bound(temp_x))
      {
      Convert(temp_x,temp_y);
      if(judge_finish())
        {
        summon();
       }
      }
    }

    void left()
    {
      int temp_x[4],temp_y[4];
      for (int i = 0; i < 4; i ++)
      temp_y[i] = tetris_store_y[i];
      for (int i = 0; i < 4; i ++)
      {
      temp_x[i] = tetris_store_x[i] - 1;
      }
      if(judge_bound(temp_x))
      {
      Convert(temp_x,temp_y);
      if(judge_finish())
        {
        summon();
       }
      }
    }
};

class maze_game//走迷宫类
{
  private:
      int maze_Map[MAX+1][MAX + 1] = {0};
      int maze_temp[MAX+1][MAX+ 1] = {0};
      bool maze_store[MAX+1][MAX+ 1][MAX*MAX] = {0};
      int maze_temp_ = 0;
      int point_coordinate[2] = {0};
      bool maze_Status = false;
      void generate() {
    randomSeed(millis());
    recursion(0,0,1);
    while(maze_Map[MAX-1][MAX-1] == 1) {
      for (int j = 0; j <= MAX; j++)
            for (int i = 0; i <= MAX; i++)
              maze_Map[i][j] = 1;
      for (int j = 0; j <= MAX; j++)
            for (int i = 0; i <= MAX; i++)
              maze_temp[i][j] = 0;
      maze_Map[0][0] = 0;
      maze_temp_ = 0;
      recursion(0,0,1);
    }
    appear();
    for (int N = 1; N <= maze_temp_; N ++) {
      for (int i = 0;i < MAX; i++) {
        for (int j = 0; j < MAX; j++) {
          maze_Map[i][j] = maze_store[i][j][N];
        }
      }
      Draw();
      delay(50);
    }
  }
  void recursion(int x,int y,int dir) {
    int num = 0;
    maze_temp_ ++;
    for (int i = 0; i < MAX; i++) {
      for (int j = 0; j < MAX; j++) {
        maze_store[i][j][maze_temp_] = maze_Map[i][j];
        if (maze_Map[i][j] == 0)
                num++;
      }
    }
    if (num>=(MAX*MAX)/2&&maze_Map[MAX-1][MAX-1] == 0) {
      return;
    }
    int temp_rand[5] = {8,8,8,8,8};
    bool re_status = true;
    while (re_status) {
      re_status = false;
      temp_rand[0] = random() % 4;
      temp_rand[1] = random() % 4;
      temp_rand[2] = random() % 4;
      temp_rand[3] = random() % 4;
      for (int i = 0; i <= 3; i++) {
        for (int j = i + 1; j <= 4; j++) {
          if (temp_rand[i] == temp_rand[j]) {
            re_status = true;
          }
        }
      }
    }
    for (int i = 0; i <= 3; i++) {
      if (temp_rand[i] == 0&&x + 1 < MAX && maze_temp[y][x + 1] != 3 && maze_Map[y][x + 1] != 0 && (maze_Map[y + 1][x + 1] + maze_Map[y][x + 2] + maze_Map[y - 1][x + 1] + maze_Map[y][x]) == 3) {
        maze_Map[y][x + 1] = 0;
        recursion(x + 1, y, 1);
      }
      if (temp_rand[i] == 1&&x > 0 && maze_temp[y][x - 1] != 3 && maze_Map[y][x - 1] != 0 && (maze_Map[y + 1][x - 1] + maze_Map[y][x] + maze_Map[y - 1][x - 1] + maze_Map[y][x - 2]) == 3) {
        maze_Map[y][x - 1] = 0;
        recursion(x - 1, y, 2);
      }
      if (temp_rand[i] == 2&&y + 1 < MAX && maze_temp[y + 1][x] != 3 && maze_Map[y + 1][x] != 0 && (maze_Map[y + 1][x + 1] + maze_Map[y + 1][x - 1] + maze_Map[y][x] + maze_Map[y + 2][x]) == 3) {
        maze_Map[y + 1][x] = 0;
        recursion(x, y + 1, 3);
      }
      if (temp_rand[i] == 3&&y > 0) {
        if (y - 1==0&&maze_temp[y - 1][x] != 3 && maze_Map[y - 1][x] != 0 && (maze_Map[y][x] + maze_Map[y - 1][x - 1] + maze_Map[y - 1][x + 1]) == 2) {
          maze_Map[y - 1][x] = 0;
          recursion(x, y - 1, 4);
        }
        if (y-1>0&&maze_temp[y - 1][x] != 3 && maze_Map[y - 1][x] != 0 && (maze_Map[y][x] + maze_Map[y - 2][x] + maze_Map[y - 1][x - 1] + maze_Map[y - 1][x + 1]) == 3) {
          maze_Map[y - 1][x] = 0;
          recursion(x, y - 1, 4);
        }
      }
    }
    if (dir == 1&&x>0) {
      maze_temp[y][x] = 3;
      x--;
      return;
    }
    if (dir == 2&&x+1<MAX) {
      maze_temp[y][x] = 3;
      x++;
      return;
    }
    if (dir == 3&&y>0) {
      maze_temp[y][x] = 3;
      y--;
      return;
    }
    if (dir == 4&&y+1<MAX) {
      maze_temp[y][x] = 3;
      y++;
      return;
    }
  }
  void miss() {
    delay(500);
    for (int i = 0; i <= 7; i ++) {
      lc. setColumn(0,7-i,B00000000);
      delay(50);
    }
    delay(500);
    for (int N = 0; N <= maze_temp_; N ++)
        for (int i = 0;i < MAX; i++)
          for (int j = 0; j < MAX; j++)
            maze_store[i][j][N] = 0;
    maze_temp_ = 0;
    for (int j = 0; j <= MAX; j++)
        for (int i = 0; i <= MAX; i++)
          maze_Map[i][j] = 1;
    for (int j = 0; j <= MAX; j++)
        for (int i = 0; i <= MAX; i++)
          maze_temp[i][j] = 0;
    maze_Map[0][0] = 0;
  }
  void appear() {
    for (int i = 7; i >=0; i --) {
      lc. setColumn(0,i,B11111111);
      delay(50);
    }
  }
  void Draw() {
    for (int j = 0; j <=7; j ++) {
      int temp = 0;
      for (int i = 0; i <= 7; i ++) {
        if (maze_Map[j][i]) {
          temp += pow(2,7-i);
        }
        if (i == 7) {
          lc. setColumn(0,j,temp);
        }
      }
    }
  }
  public:
         void point_twinkle() {
    int maze_temp_map[MAX][MAX];
    for (int i = 0; i < MAX; i ++)
          for (int j = 0; j < MAX; j ++)
            maze_temp_map[i][j] = maze_Map[i][j];
    if (maze_Status) {
      maze_temp_map[point_coordinate[0]][point_coordinate[1]] = 1;
    }
    for (int j = 0; j <=7; j ++) {
      int temp = 0;
      for (int i = 0; i <= 7; i ++) {
        if (maze_temp_map[j][i]) {
          temp += pow(2,7-i);
        }
        if (i == 7) {
          lc. setColumn(0,j,temp);
        }
      }
    }
    maze_Status = !maze_Status;
  }
  void point_move(int dir,char axis) {
    if (axis == 'x'&&(point_coordinate[1]+dir)>=0&&(point_coordinate[1]+dir)<= MAX-1) {
      if (!(maze_Map[point_coordinate[0]][point_coordinate[1]+dir]))
            point_coordinate[1] = point_coordinate[1]+dir;
    }
    if (axis == 'y'&&(point_coordinate[0]+dir)>=0&&(point_coordinate[0]+dir)<= MAX-1) {
      if (!(maze_Map[point_coordinate[0]+dir][point_coordinate[1]]))
            point_coordinate[0] = point_coordinate[0]+dir;
    }
    if (point_coordinate[0] == MAX-1 &&point_coordinate[1] == MAX-1 ) {
      point_coordinate[0] = 0;
      point_coordinate[1] = 0;
      miss();
      generate();
      
    }
  }
  void Start() {
    game_mode = 1;
    point_coordinate[0] = 0;
    point_coordinate[1] = 0;
    miss();
    generate();
  }
};

tetris_game tetris;//创建游戏对象
snake_game snake;
maze_game maze;

void heart_beat();
int get_weather();
void Auto_time();
void handleRoot();
void handlemaze();
void handlesnake();
void handletetris();
void handleheart();
void handlesleep();
void handlelumi();
void handleTiming();
void handleWeather();
void handlereset();
void handleWifi();
void handleAmap();
void handleAp();
void EEPROM_write(String a,String b,int EEPROM_write_Start);
String EEPROM_read_a(int Start);
String EEPROM_read_b(int Start);
bool Internet_status();
void draw_on_screen();
void draw_on_screen(int temp_font[],int screen_num);

int minu  = 0,hour = 0;
String wifi_information[2] = {"",""};//wifi信息
String Amap_information[2] = {"",""};//高德开发者信息
String ap_information[2] = {"",""};//AP信息
String Word_information = "HELLO WORLD";//字符动画默认字符串
int sleep_information[2] = {24,24};//休眠设定
bool sleep_status = true;
int draw_temp[8] = {0};//用于滚动动画
const char  * host  = "restapi.amap.com";

int NUMBER[10][8] =
{
  { 0x00, 0x07, 0x05, 0x05, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x07, 0x04, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x07, 0x01, 0x07, 0x00, 0x0F },
  { 0x00, 0x05, 0x05, 0x07, 0x01, 0x01, 0x00, 0x0F },
  { 0x00, 0x07, 0x04, 0x07, 0x01, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x04, 0x07, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x01, 0x01, 0x01, 0x00, 0x0F },
  { 0x00, 0x07, 0x05, 0x07, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x05, 0x07, 0x01, 0x01, 0x00, 0x0F },
};
int Weathericon[20][8] =
{
  { 0x08, 0x6A, 0x3E, 0xFC, 0x3F, 0x7C, 0x56, 0x10 },     /* 晴 */
  { 0x44, 0x66, 0xFF, 0xFF, 0x40, 0x12, 0x80, 0x09 },     /* 雨 */
  { 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00 },     /* 雾 */
  { 0x00, 0x08, 0x5C, 0xFE, 0xFF, 0x00, 0x00, 0x00 },     /* 阴 */
  { 0x00, 0x04, 0x2E, 0x74, 0xFE, 0xFF, 0x00, 0x00 },     /* 多云 */
  { 0x18, 0x78, 0x03, 0x7F, 0x00, 0x7C, 0x0C, 0x00 },     /* 风 */
  { 0x04, 0x4E, 0xFF, 0xFF, 0x42, 0x17, 0x82, 0x10 },     /* 夹 */
  { 0x2A, 0x5D, 0x2A, 0xFF, 0x2A, 0x5D, 0x2A, 0x08 },     /* 雪 */
  { 0x19, 0xF8, 0x0B, 0xFF, 0x04, 0xFD, 0x0C, 0x01 },     /* 沙 */
  { 0xFF, 0x7F, 0x7E, 0x3E, 0x3C, 0x1C, 0x18, 0x08 },     /* 龙卷风 */
  { 0x01, 0xAE, 0xAA, 0xEA, 0x2A, 0x2E, 0x00, 0x00 },     /* 热 */
  { 0x02, 0x1C, 0x14, 0xD4, 0x14, 0x1C, 0x00, 0x00 },     /* 冷 */
  { 0x00, 0x1C, 0x14, 0x14, 0x04, 0x00, 0x04, 0x00 },     /* 未知 */
  {0x00,0x3C,0x66,0x81,0x3C,0x42,0x18,0x18},
};

int initial[20][8] =
{
  { 0x5F, 0xE2, 0x42, 0x42, 0x62, 0xC2, 0x42, 0xC6 },//打
  { 0xFF, 0x44, 0xFE, 0x44, 0x44, 0x44, 0x44, 0x84 },//开
  { 0xAA, 0xA8, 0xAA, 0xAA, 0xAA, 0xAA, 0x52, 0x52 },//WI
  { 0xFA, 0x80, 0x82, 0xFA, 0x82, 0x82, 0x82, 0x82 },//FI
  { 0x90, 0x3E, 0xD8, 0x5E, 0x88, 0x5E, 0x88, 0x7F },//连
  { 0x44, 0xFF, 0x4A, 0x5F, 0x64, 0xDF, 0x4A, 0xCC },//接
  { 0x7E, 0x42, 0x44, 0x78, 0x50, 0x48, 0x44, 0x42 },//R
  { 0x00, 0x7E, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00 },//C
  { 0x7E, 0x7E, 0x60, 0x7E, 0x7E, 0x60, 0x60, 0x60 },
  { 0x18, 0x18, 0x66, 0x66, 0x7E, 0x7E, 0x66, 0x66 },
  { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
  { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x7E },
  { 0x0A, 0x7E, 0x48, 0x44, 0xF7, 0x94, 0xBA, 0xB1 },
  { 0x04, 0x04, 0x1F, 0xE9, 0x49, 0x69, 0xD1, 0x13 },
};

int heartbeat[27][8] =
{
    {0x00,0x1C,0x22,0x22,0x22,0x3E,0x22,0x22},//A
    {0x00,0x3C,0x22,0x22,0x3E,0x22,0x22,0x3C},//B
    {0x00,0x1C,0x22,0x20,0x20,0x20,0x22,0x1C},//C
    {0x00,0x3C,0x22,0x22,0x22,0x22,0x22,0x3C},//D
    {0x00,0x3E,0x20,0x20,0x3E,0x20,0x20,0x3E},//E
    {0x00,0x3E,0x20,0x20,0x3E,0x20,0x20,0x20},//F
    {0x00,0x1C,0x22,0x20,0x3E,0x22,0x22,0x1C},//G
    {0x00,0x22,0x22,0x22,0x3E,0x22,0x22,0x22},//H
    {0x00,0x1C,0x08,0x08,0x08,0x08,0x08,0x1C},//I
    {0x00,0x7E,0x04,0x04,0x44,0x24,0x18,0x00},//J
    {0x00,0x20,0x2C,0x30,0x20,0x30,0x2C,0x20},//K
    {0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x3E},//L
    {0x00,0x42,0x66,0x5A,0x42,0x42,0x42,0x42},//M
    {0x00,0x42,0x72,0x5A,0x4A,0x4E,0x42,0x00},//N
    {0x00,0x1C,0x22,0x22,0x22,0x22,0x22,0x1C},//O
    {0x00,0x3C,0x22,0x22,0x3C,0x20,0x20,0x20},//P
    {0x00,0x1C,0x22,0x22,0x22,0x2A,0x26,0x1F},//Q
    {0x00,0x7C,0x42,0x42,0x7C,0x50,0x48,0x44},//R
    {0x00,0x1C,0x22,0x20,0x1C,0x02,0x22,0x1C},//S
    {0x00,0x3E,0x08,0x08,0x08,0x08,0x08,0x08},//T
    {0x00,0x42,0x42,0x42,0x42,0x42,0x42,0x3C},//U
    {0x00,0x22,0x22,0x22,0x14,0x14,0x08,0x00},//V
    {0x00,0x41,0x41,0x49,0x55,0x55,0x63,0x41},//W
    {0x00,0x00,0x42,0x24,0x18,0x18,0x24,0x42},//X
    {0x00,0x42,0x24,0x18,0x10,0x10,0x10,0x00},//Y
    {0x00,0x3E,0x02,0x04,0x08,0x10,0x20,0x3E},//Z
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},//空格
};

const char PAGE_Template[] PROGMEM= R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
    body{
      background-repeat:no-repeat;
      background-size:100% auto;
      background: linear-gradient(to right, #3b7066 0%, #4ca587 100%);
      }
      #login-box{
      width:40%;
      height:auto;
      margin:0 auto ;
      margin-top:13%;
      text-align:center;
      background:#00000060;
      padding:20px 50px;
      }
    #login-box {
      color:#fff;
    }
    #login-box .form .item{
      margin-top:15px;
    }
    #login-box .form .item i{
      font-size:18px;
    }
    #login-box .form .item input{
      width:180px;
      font-size:18px;
      border:0;
      border-bottom:2px solid #fff;
      padding:5px 10px;
    }
    #login-box  button{
      margin-top:20px;
      width:190px;
      height:30px;
      font-size:20px;
      font-weight:700;
      color:#fff;
      background-image: linear-gradient(to right, #4ca587 0%, #3b7066 100%);
      color:#fff;
      border:0;
      border-radius:5px;
    }
</style>
</head>
<body>
<div id="login-box">
)=====";


const char PAGE_Wifi[] PROGMEM= R"=====(
    <h1>WiFi信息</h1>
    <form action="" method="GET">
    <div class="form">
      <div class="item">
          <i style="font-size:20px; color:#fff">wifi名称</i>
         <input type="text" name="ssid">
      </div>
      <div class="item">
          <i style="font-size:20px; color:#fff">wifi密码</i>
         <input type="text" name="password">    
      </div>
    </div>
    <button type="submit" value="Submit">连接</button>
    </form>
  </div>
</body>
</html>
)=====";

const char PAGE_Amap[] PROGMEM= R"=====(
    <h1>高德天气配置</h1>
    <form action="" method="GET">
    <div class="form">
      <div class="item">
      <i style="font-size:20px; color:#fff">高德KEY</i>
         <input type="text" name="key">
      </div>
      <div class="item">
      <i style="font-size:20px; color:#fff">城市代码</i>
         <input type="text" name="city">
      </div>
    </div>
    <button type="submit" value="Submit">提交</button>
    </form>
  </div>
</body>
</html>
)=====";

const char PAGE_Ap[] PROGMEM= R"=====(
    <h1>热点设置</h1>
    <form action="" method="GET">
    <div class="form">
      <div class="item">
          <i style="font-size:20px; color:#fff">热点名称 </i>
         <input type="text" name="ap_name">
      </div>
      <div class="item">
          <i style="font-size:20px; color:#fff">热点密码</i>
         <input type="text" name="ap_password">    
      </div>
    </div>
    <button type="submit" value="Submit">提交</button>
    </form>
  </div>
</body>
</html>
)=====";

const char PAGE_Sleep[] PROGMEM= R"=====(
<h1>休眠设置(不需要休眠请都填写24,并将包含选择“否”)</h1>
    <form action="" method="GET">
    <div class="form">
      <div class="item">
          <p style="font-size:20px; color:#fff">起始时间(0-23)</p>
         <input type="text" name="sleep_begin">
         
      </div>
      <div class="item">
          <p style="font-size:20px; color:#fff">结束时间(0-23)</p>
         <input type="text" name="sleep_end">
      </div>
     <br>此时是否包含在设置时间内
      <input type="radio" name="now" value="1">是
      <input type="radio" name="now" value="0" checked="checked">否
    </div>
    <button type="submit" value="Submit">提交</button>
    </form>
  </div>
</body>
</html>
)=====";

const char PAGE_heart[] PROGMEM= R"=====(
    <h1>字符动画</h1>
    <form action="" method="GET">
    <div class="form">
      <div class="item">
          <p style="font-size:20px; color:#fff">输入字符串(只支持大写字母)</p>
         <input type="text" name="word">
      </div>
    </div>
    <button type="submit" value="Submit">提交</button>
    </form>
  </div>
</body>
</html>
)=====";

const char PAGE_Lumi[] PROGMEM= R"=====(
  <style>
  .selecter{  
      background:#fafdfe;  
      height:28px;  
      width:180px;  
      line-height:28px;  
      border:1px solid #9bc0dd;  
      -moz-border-radius:2px;  
      -webkit-border-radius:2px;  
      border-radius:2px;  
    }
    </style>
  <script type="text/javascript"> 
        function submit(){ 
        var jump = '?lumi=' + String(lumi.value);
                window.location= jump;
        } 
</script>
    <h1>显示屏亮度</h1>
    <div class="form">
     <div class="item">
      <select name="lumi" id="lumi" class="selecter">
        <option value="0">0</option>
        <option value="1">1</option>
        <option value="2">2</option>
        <option value="3">3</option>
        <option value="4">4</option>
        <option value="5">5</option>
        <option value="6">6</option>
        <option value="7">7</option>
        <option value="8">8</option>
      </select>
      </div>
      <button type="submit" value="Submit" onclick="submit(this)">提交</button>
    </div>
  </div>
</body>
</html>
)=====";

const char PAGE_Root[] PROGMEM= R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>主页</title>
<style>
    body{
      background-repeat:no-repeat;
      background-size:100% auto;
    background: linear-gradient(to right, #3b7066 0%, #4ca587 100%);
      }
      #login-box{
    text-align:center;
      width:60%;
      height:auto;
      margin:0 auto;
      margin-top:10%;
      background:#00000060;
      padding:20px 50px;
    color:#fff;
      }
    #login-box .form{
    display: flex;
    justify-content: center;
    }
  #login-box .form div{
    margin:10px;
    }
    #login-box .form .item{
      margin-top:10px;
    }
    #login-box .form .item input{
      width:180px;
      font-size:18px;
      border:0;
      border-bottom:2px solid #fff;
      padding:5px 10px;
    }
    #login-box  button{
      margin-top:20px;
      width:190px;
      height:30px;
      font-size:20px;
      font-weight:700;
      color:#fff;
      background-image: linear-gradient(to right, #4ca587 0%, #3b7066 100%);
      color:#fff;
      border:0;
      border-radius:5px;
    }
</style>
</head>
<body>
  <div id="login-box">
    <h1>主页</h1>
    <div class="form">
    <div class="normal">
      <h3>基础设置<h3>
      <div class="item">
       <a href="./wifi"><button>wifi设置</button></a>
      </div>
      <div class="item">
       <a href="./amap"><button>天气设置</button></a>
      </div>
      <div class="item">
       <a href="./ap"><button>热点设置</button></a>
      </div>
      <div class="item">
       <a href="./lumi"><button>显示屏亮度</button></a>
      </div>
    </div>
    <div class="other">
      <h3>特殊<h3>
      <div class="item">
       <a href="./sleep"><button>息屏时间</button></a>
      </div>
      <div class="item">
       <a href="./Timing"><button>校时</button></a>
      </div>
      <div class="item">
       <a href="./Weather"><button>校对天气</button></a>
      </div>
      <div class="item">
       <a href="./reset"><button>初始化</button></a>
      </div>
    </div>
  <div class="other">
      <h3>小游戏<h3>
      <div class="item">
       <a href="./maze"><button>走迷宫</button></a>
      </div>
      <div class="item">
       <a href="./snake"><button>贪吃蛇</button></a>
      </div>
      <div class="item">
       <a href="./tetris"><button>俄罗斯方块</button></a>
      </div>
      <div class="item">
       <a href="./heart"><button>字符动画</button></a>
      </div>
    </div>
    </div>
  </div>
</body>
</html>
</html>
)=====";

const char PAGE_game_template[] PROGMEM= R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
    body{
      background-repeat:no-repeat;
      background-size:100% auto;
    background: linear-gradient(to right, #3b7066 0%, #4ca587 100%);
      }
      #login-box{
    text-align:center;
      width:60%;
      height:auto;
      margin:0 auto;
      margin-top:10%;
      background:#00000060;
      padding:20px 50px;
    color:#fff;
      }
    #login-box .form{
    display: flex;
    justify-content: center;
    }
  #login-box .form div{
    margin:10px;
    }
    #login-box .form .item{
      margin-top:10px;
    }
    #login-box .form .item input{
      width:180px;
      font-size:18px;
      border:0;
      border-bottom:2px solid #fff;
      padding:5px 10px;
    }
    #login-box  button{
      margin-top:20px;
      width:190px;
      height:190px;
      font-size:20px;
      font-weight:700;
      color:#fff;
      background-image: linear-gradient(to right, #4ca587 0%, #3b7066 100%);
      color:#fff;
      border:0;
      border-radius:5px;
    }
</style>
</head>
)=====";

const char PAGE_maze[] PROGMEM= R"=====(
<script>
function up()
{select("u");}
function down()
{select("d");}
function right()
{select("r");}
function left()
{select("l");}
function select(message)
{
  xmlhttp=new XMLHttpRequest();
  var url =  "/maze?position=" + message;
  xmlhttp.open("GET",url,true);
  xmlhttp.send();
}
</script>
<body>
  <div id="login-box">
    <h1>走迷宫</h1>
    <div class="form">
    <div class = "item">
    <br>
  <br>
  <br>
    <br>
  <br>
    <br>
    <br>
    <button type="button" onclick="up()">左</button>
    </div>
    <div class="normal">
    <div class="item">
    <button type="button" onclick="left()">上</button>
    </div>
    <br>
    <br>
    <div class="item">
    <button type="button" onclick="right()">下</button>
    </div>
    </div>
    <div class="item">
    <br>
    <br>
  <br>
  <br>
    <br>
    <br>
  <br>
    <button type="button" onclick="down()">右</button>
    </div>
    </div>
  </div>
</body>
</html>
</html>
)=====";

const char PAGE_tetris[] PROGMEM= R"=====(
<script>
function right()
{select("r");}
function left()
{select("l");}
function rotate()
{ select("o");}
function select(message)
{
  xmlhttp=new XMLHttpRequest();
  var url =  "/tetris?position=" + message;
  xmlhttp.open("GET",url,true);
  xmlhttp.send();
}
</script>
<body>
  <div id="login-box">
    <h1>俄罗斯方块</h1>
    <div class="form">
    <div class = "item">
    <button type="button" onclick="left()">左</button>
    <button type="button" onclick="right()">右</button>
    </div>
    </div>
  <div class = "item">
  <button type="button" onclick="rotate()">旋转</button>
  </div>
  </div>
</body>
</html>
</html>
)=====";

const char PAGE_snake[] PROGMEM= R"=====(
<script>
function up()
{select("u");}
function down()
{select("d");}
function right()
{select("r");}
function left()
{select("l");}
function easy()
{select("e");}
function normal()
{select("n");}
function hard()
{ select("h");}
function infinite()
{select("f");}
function limited()
{ select("m");}
function select(message)
{
  xmlhttp=new XMLHttpRequest();
  var url =  "/snake?position=" + message;
  xmlhttp.open("GET",url,true);
  xmlhttp.send();
}
</script>
<body>
  <div id="login-box">
    <h1>贪吃蛇</h1>
    <div class="form">
    <div class = "item">
    <br>
  <br>
  <br>
    <br>
  <br>
    <br>
    <br>
    <button type="button" onclick="left()">左</button>
    </div>
    <div class="normal">
    <div class="item">
    <button type="button" onclick="up()">上</button>
    </div>
    <br>
    <br>
    <div class="item">
    <button type="button" onclick="down()">下</button>
    </div>
    </div>
    <div class="item">
    <br>
    <br>
  <br>
  <br>
    <br>
    <br>
  <br>
    <button type="button" onclick="right()">右</button>
    </div>
    </div>
  <button type="button" onclick="easy()">简单</button>
  <button type="button" onclick="normal()">普通</button>
  <button type="button" onclick="hard()">困难</button>
  <button type="button" onclick="infinite()">地图无墙</button>
  <button type="button" onclick="limited()">地图有墙</button>
  </div>
</body>
</html>
</html>
)=====";






void setup() {
  Serial.begin(9600);
  EEPROM.begin(1024);
  WiFi.mode(WIFI_AP_STA);
  server.on( "/amap", handleAmap );
  server.on( "/Timing", handleTiming );
  server.on( "/maze", handlemaze );
  server.on( "/snake", handlesnake );
  server.on( "/tetris", handletetris );
  server.on( "/heart", handleheart );
  server.on( "/Weather", handleWeather );
  server.on( "/lumi", handlelumi );
  server.on( "/ap", handleAp );
  server.on( "/wifi", handleWifi );
  server.on( "/", handleRoot );
  server.on( "/reset", handlereset );
  server.on( "/sleep", handlesleep );
  server.begin();
  for ( int i = 0; i <= 3; i++ )//显示屏初始化
    {
      lc.shutdown( i, false );
      lc.clearDisplay( i );
      lc.setIntensity( i, 1 );
    }
    if (EEPROM.read(1023) == 255)
    {
      for (int i = 0; i <= 500; i ++)
      {
        if (EEPROM.read(i) != 255)
        {
          EEPROM.write(i,255);
        }
      }
      EEPROM.commit();
      String apName = ("RC");//wifi初始化
      WiFi.softAP( apName, "" );
      for ( int i = 0; i <= 7; i++ )
      {
        lc.setRow( 3, i, initial[0][i] );
        lc.setRow( 2, i, initial[1][i] );
        lc.setRow( 1, i, initial[2][i] );
        lc.setRow( 0, i, initial[3][i] );
      }
      delay( 2000 );
      for ( int i = 0; i <= 3; i++ )
        lc.clearDisplay( i );
      for ( int i = 0; i <= 7; i++ )
      {
        lc.setRow( 3, i, initial[4][i] );
        lc.setRow( 2, i, initial[5][i] );
        lc.setRow( 1, i, initial[6][i] );
        lc.setRow( 0, i, initial[7][i] );
      }
      delay( 2000 );
       for ( int i = 0; i <= 3; i++ )
        lc.clearDisplay( i );
        for ( int i = 3; i >= 0; i-- )
        {
          lc.setRow( i, 7, B00000001 );
          delay( 200 );
        }
        draw_on_screen(NUMBER[1],3);
      while ( !Internet_status() )
      {
        server.handleClient();
      }
        draw_on_screen(NUMBER[2],2);
      while (EEPROM.read(AMAP_CONFIG) == 255||EEPROM.read(AMAP_CONFIG+1) == 255)
      {
        server.handleClient();
      }
        draw_on_screen(NUMBER[3],1);
      while (EEPROM.read(AP_CONFIG) == 255||EEPROM.read(AP_CONFIG+1) == 255)
      {
        server.handleClient();
      }
      EEPROM.write(1023,1);
      EEPROM.commit();
  }
  else
  {
    WiFi.softAP(EEPROM_read_a(AP_CONFIG),EEPROM_read_b(AP_CONFIG));
    wifi_information[0] = EEPROM_read_a(WIFI_CONFIG);
    wifi_information[1] = EEPROM_read_b(WIFI_CONFIG);
    ap_information[0] = EEPROM_read_a(AP_CONFIG);
    ap_information[1] = EEPROM_read_b(AP_CONFIG);
    Amap_information[0] = EEPROM_read_a(AMAP_CONFIG);
    Amap_information[1] = EEPROM_read_b(AMAP_CONFIG);
    if (EEPROM.read(WORD_CONFIG) != 255)
    {
      Word_information = EEPROM_read_a(WORD_CONFIG);
    }
    do{
        for ( int i = 0; i <= 3; i++ )
          lc.clearDisplay( i );
        for ( int i = 3; i >= 0; i-- )
        {
          lc.setRow( i, 7, B00000001 );
          delay( 200 );
        }
      }while ( !Internet_status() );
       server.handleClient();
   }
   delay(500);
  TimeClient.begin();
  TimeClient.update();
  minu  = TimeClient.getMinutes();
  hour  = TimeClient.getHours();
  draw_on_screen();
  draw_on_screen(Weathericon[get_weather()],1);
}

void loop() 
{
  server.handleClient();
  if (time_tick.check(1))
  {
    Auto_time();
    draw_on_screen();
  }
  if (weather_tick.check(1))
  {
    draw_on_screen(Weathericon[get_weather()],1);
  }
  if (game_tick.check(0))
  {
    switch(game_mode)
    {
      case 0:
        heart_beat();
        break;
      case 1:
        maze.point_twinkle();
        break;
      case 2:
        snake.AutoMove();
        break;
      case 3:
        tetris.AutoMove();
        break;
    }
  }
}

void heart_beat()
{
  for (int i = 0; i <= Word_information.length(); i ++)
  {
    if (Word_information[i] == 32|| i == Word_information.length())
    {
      draw_on_screen(heartbeat[26],0);
      continue;
    }
    if (game_mode != 0) break;
    draw_on_screen(heartbeat[Word_information[i]-65],0);
  }
}

int get_weather()
{
  int temp    = 13;
  String  comp[30]  = { "晴", "0", "云", "4", "阴", "3", "风", "5", "霾", "2", "雨", "1", "夹", "6", "雪", "7", "沙", "8", "卷", "9", "雾", "2", "热", "10", "冷", "11", "未", "12" };//伪字典
  WiFiClient client;
  client.connect( host, 80 );
  String request = (String) ("GET ") + "/v3/weather/" + "weatherInfo?" + "city=" + Amap_information[1] + "&key=" + Amap_information[0] + " HTTP/1.1\r\n" +
       "Content-Type: text/html;charset=utf-8\r\n" +
       "Host: " + host + "\n" +
       "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.70" +
       "Connection: Keep Alive\r\n\r\n";
  client.print( request );
  String response = client.readString();
  client.stop();
  for ( int i = 0; i <= 27; i = i + 2 )//匹配天气
  {
    if ( response.indexOf( comp[i] ) != -1 )
    {
      temp = comp[i + 1].toInt();
      break;
    }
  }
  return temp;
}

void Auto_time()
{
  minu ++;
  if (minu == 60)
  {
    hour ++;
    if (hour == 24)
    {
        hour = 0;
    }
    if (hour == sleep_information[1]||hour == sleep_information[0])
    {
      sleep_status = !sleep_status;
    }
    minu = 0;
  }
   if (sleep_status)
   {
            for (int i = 0; i <= 3; i ++)
              lc.shutdown( i, false );
   }
   else
   {
           for (int i = 0; i <= 3; i ++)
             lc.shutdown( i, true );
   }
}

void handleRoot()
{
  String html = PAGE_Root;
  if (EEPROM.read(1023) == 255)
  {
    server.send(200, "text/html", "<center><h1>请先进行基本设置</h1></center>"+html);
  }
  else
  {
    server.send(200, "text/html", html);
  }
}

void handlemaze()
{
  String game_template = PAGE_game_template;
  String web = PAGE_maze;
  server.send(200, "text/html", game_template+web);
  if (game_mode != 1)
    maze.Start();
  String message = server.arg("position");
  char temp[message.length()+1];
  Serial.println(1);
  strcpy(temp,message.c_str());
  if (temp[0] == 'u') {
    maze.point_move(-1,'y');
  }
  if (temp[0] == 'd') {
    maze.point_move(1,'y');
  }
  if (temp[0] == 'r') {
    maze.point_move(1,'x');
  }
  if (temp[0] == 'l') {
    maze.point_move(-1,'x');
  }
}

void handlesnake()
{
  if (game_mode!=2)
  {
    game_mode = 2;
    snake.Start();
  }
  String game_template = PAGE_game_template;
  String web = PAGE_snake;
  server.send(200, "text/html", game_template+web);
  String message = server.arg("position");
  char temp[message.length()+1];
  strcpy(temp,message.c_str());
  if (temp[0] =='u'||temp[0]=='d'||temp[0]=='l'||temp[0]=='r'||temp[0] =='p') {snake.snake_Direction = temp[0];}
  if (temp[0] =='e') {snake.snake_form = 0;}
  if (temp[0] =='n') {snake.snake_form = 15;}
  if (temp[0] =='h') {snake.snake_form = 255;}
  if (temp[0] == 'f') {snake.snake_map_infinity= true;}
  if (temp[0] == 'm') {snake.snake_map_infinity= false;}
}

void handletetris()
{
  if (game_mode!=3)
  {
    tetris.Start();
  }
  tetris.tetris_Moving = false;
  String game_template = PAGE_game_template;
  String web = PAGE_tetris;
  server.send(200, "text/html", game_template+web);
  String message = server.arg("position");
  char temp[message.length()+1];
  strcpy(temp,message.c_str());
  if (tetris.tetris_Position)
  {
     if (temp[0] == 'r') {tetris.right();}
     if (temp[0] == 'l') {tetris.left();}
     if (temp[0] == 'o') {tetris.Rotate();}
  }
    tetris.tetris_Moving = true;
}

void handleheart()
{
  game_mode = 0;
  String html = PAGE_heart;
  String Template = PAGE_Template;
  server.send(200, "text/html",Template + html+"<center><h2>当前字符串: "+Word_information +"</h2></center>");
  if (server.hasArg("word"))
  {
    String word_string = server.arg("word");
    EEPROM_write(word_string,"",WORD_CONFIG);
    Word_information = word_string;
  }
}

void handlesleep()
{
  String html = PAGE_Sleep;
  String Template = PAGE_Template;
  server.send(200, "text/html",Template + html);
  if (server.hasArg("sleep_begin")&&server.hasArg("sleep_end"))
  {
  String  sleep_begin = server.arg( "sleep_begin" );
  String  sleep_end = server.arg( "sleep_end" );
  String sleep_now = server.arg( "now" );
  if (sleep_now == "1") {sleep_status = false;}
  else {sleep_status = true;}
  sleep_information[0] = sleep_begin.toInt();
  sleep_information[1] = sleep_end.toInt();
  }
}

void handlelumi()
{
  String html = PAGE_Lumi;
  String Template = PAGE_Template;
  server.send(200, "text/html",Template + html);
  if (server.hasArg("lumi"))
  {
  String  lumi = server.arg( "lumi" );
  for ( int i = 0; i <= 3; i++ )
    {
      lc.setIntensity( i, lumi.toInt() );
    }
  }
}

void handleTiming()
{
  if (Internet_status())
  {
    TimeClient.update();
    minu  = TimeClient.getMinutes();
    hour  = TimeClient.getHours();
    draw_on_screen();
    server.send(200, "text/html", "<center><h1>校时成功</h1></center>");
  }
    server.send(200, "text/html", "<center><h1>校时失败，请检查网络情况</h1></center>");
}

void handleWeather()
{
  if (Internet_status())
  {
    game_mode = 4;
    for (int i = 0; i <= 7 ; i ++)
    {
      lc.setRow( 1, i,Weathericon[get_weather()][i] );
    }
    game_mode = 0;
    server.send(200, "text/html", "<center><h1>校对天气成功</h1></center>");
  }
  else
    server.send(200, "text/html", "<center><h1>校对天气失败，请检查网络情况</h1></center>");
}

void handlereset()
{
  EEPROM.write(1023,255);
  EEPROM.commit();
  server.send(200, "text/html", "<h1>初始化完成请重启单片机</h1>");
}

void handleWifi()
{
  String html = PAGE_Wifi;
  String Template = PAGE_Template;
  server.send(200, "text/html",Template+ html + "<center><h2>当前SSID:"+wifi_information[0] + "          PASSWORD:"+wifi_information[1]+"</h2></center>");
  if (server.hasArg("ssid")&&server.hasArg("password"))
  {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  EEPROM_write(ssid,password,WIFI_CONFIG);
  wifi_information[0] = EEPROM_read_a(WIFI_CONFIG);
  wifi_information[1] = EEPROM_read_b(WIFI_CONFIG);
  }
}

void handleAmap()
{
  String html = PAGE_Amap;
  String Template = PAGE_Template;
  server.send(200, "text/html", Template+html+"<center><h2>当前KEY:"+Amap_information[0] + "          CITY:"+Amap_information[1]+"</h2></center>");
  if (server.hasArg("key")&&server.hasArg("city"))
  {
  String key = server.arg("key");
  String city = server.arg("city");
  EEPROM_write(key,city,AMAP_CONFIG);
  Amap_information[0] = EEPROM_read_a(AMAP_CONFIG);
  Amap_information[1] = EEPROM_read_b(AMAP_CONFIG);
  draw_on_screen(Weathericon[get_weather()],1);
  }
}

void handleAp()
{
  String html = PAGE_Ap;
  String Template = PAGE_Template;
  server.send(200, "text/html",Template+ html+"<center><h2>当前AP_NAME:"+ap_information[0] + "          AP_PASSWORD:"+ap_information[1]+"</h2></center>");
  if (server.hasArg("ap_name")&&server.hasArg("ap_password"))
  {
  String ap_name = server.arg("ap_name");
  String ap_password = server.arg("ap_password");
  EEPROM_write(ap_name,ap_password,AP_CONFIG);
  ap_information[0] = EEPROM_read_a(AP_CONFIG);
  ap_information[1] = EEPROM_read_b(AP_CONFIG);
  }
}

void EEPROM_write(String a,String b,int EEPROM_write_Start)
{
  EEPROM.write(EEPROM_write_Start,a.length());
  EEPROM.write(EEPROM_write_Start+1,b.length());
  for (int i = EEPROM_write_Start + 2; i < EEPROM_write_Start + 2 + a.length(); i ++)
  {
    EEPROM.write(i,a[i - EEPROM_write_Start - 2]);
  }
  for (int i = EEPROM_write_Start + 2 + a.length(); i < EEPROM_write_Start + 2 + a.length()+b.length(); i ++)
  {
    EEPROM.write(i,b[i - EEPROM_write_Start - 2 - a.length()]);
  }
   EEPROM.commit();
}

String EEPROM_read_a(int Start)
{
  String EEPROM_read_a_temp = "";
  for (int i = Start + 2; i < Start + 2 + int(EEPROM.read(Start)); i ++)
      {
        EEPROM_read_a_temp += char(EEPROM.read(i));
      }
      return EEPROM_read_a_temp;
}

String EEPROM_read_b(int Start)
{
  String EEPROM_read_b_temp="";
  for (int i =Start + 2 + EEPROM.read(Start); i < Start + 2+int(EEPROM.read(Start))+int(EEPROM.read(Start + 1)); i ++)
      {
        EEPROM_read_b_temp += char(EEPROM.read(i));
      }
      return EEPROM_read_b_temp;
}

bool Internet_status()
{
  WiFi.begin(wifi_information[0],wifi_information[1]);
  delay(5000);
  WiFiClient client;
  HTTPClient webtest;
   webtest.begin(client,"http://baidu.com");
  int Internet_status_code = webtest.GET();
  Serial.println(Internet_status_code);
  if (Internet_status_code == 200)
  {
    return true;
  }
  else
  {
    return false;
  }
  webtest.end();
}

void draw_on_screen()
{
        for ( int i = 0; i <= 7; i++ )
        {
          lc.setRow( 3, i, NUMBER[hour / 10][i] * pow( 2, 5 ) + NUMBER[hour - (hour / 10) * 10][i] * 2 );
          lc.setRow( 2, i, NUMBER[minu / 10][i] * pow( 2, 4 ) + NUMBER[minu - (minu / 10) * 10][i] );
        }
        lc.setRow( 3, 7, B11111111 );
}

void draw_on_screen(int temp_font[],int screen_num)
{
      for ( int i = 0; i <= 7; i++ )
      {
        for (int j = 7; j >= 0 ; j --)
        {
          if (i - j>= 0)
            lc.setRow( screen_num, 7-j,temp_font[i-j] );
          else
            lc.setRow( screen_num, 7-j,draw_temp[j-i-1] );
        }
        server.handleClient();
        delay(150);
      }
      for (int i = 0; i <= 7 ; i ++)
      {
        draw_temp[7-i] = temp_font[i];
      }
}