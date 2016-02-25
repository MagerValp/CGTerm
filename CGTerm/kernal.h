#define COLOR_BLACK	0
#define COLOR_WHITE	1
#define COLOR_RED	2
#define COLOR_CYAN	3
#define COLOR_PURPLE	4
#define COLOR_GREEN	5
#define COLOR_BLUE	6
#define COLOR_YELLOW	7
#define COLOR_ORANGE	8
#define COLOR_BROWN	9
#define COLOR_LTRED	10
#define COLOR_DKGRAY	11
#define COLOR_GRAY	12
#define COLOR_LTGREEN	13
#define COLOR_LTBLUE	14
#define COLOR_LTGRAY	15


extern unsigned char screencode[];


int kernal_init(void);
void ffd2(unsigned char a);
void print(unsigned char *s);
void print_ascii(unsigned char *s);
unsigned char ffe4(void);
