#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include <sys/types.h>


#define BUFCAP 1024
typedef float ValType ;
ValType val_buf[BUFCAP];
size_t val_buf_index = 0;
size_t buf_size = 0;


#define STRBUFCAP 2048
char str_buf[STRBUFCAP];

float map(float val, float old_low, float old_high, float new_low, float new_high){  
        float old_range =  old_high - old_low;                                                            
        float new_range = new_high - new_low;                                                             
        float factor = new_range / old_range;                                                             
        val -= old_low;  
        val *= factor; 
        val += new_low;   
        return val;  
}      

void insert_buf(ValType val){
	val_buf[val_buf_index] = val;
	val_buf_index = (val_buf_index + 1) % BUFCAP;
	if(buf_size < BUFCAP){
		buf_size++;
	}
}

ValType parse_val(){
	ValType val;
	int res = sscanf(str_buf,"%f\n",&val);
	if(!res){ exit(1); }
	return val;
}

void read_stream(FILE* stream){
	char c = 0;
	size_t str_i = 0;
	while(c != EOF && str_i < STRBUFCAP){
		c = fgetc(stream);
		if(c == '\n'){
			ValType val = parse_val();
			insert_buf(val);
			str_i = 0;
			str_buf[str_i] = '\0';
			return;
		}else if(c != EOF){
			str_buf[str_i] = c;
			str_i++;
			str_buf[str_i] = '\0';
		}
	}
}

ValType get_buf_max(){
	ValType max = FLT_MIN;
	for(size_t i = 0; i < buf_size; i++){
		if(val_buf[i] > max){
			max = val_buf[i];
		}
	}
	return max;
}

ValType get_buf_min(){
	ValType min = FLT_MAX;
	for(size_t i = 0; i < buf_size; i++){
		if(val_buf[i] < min){
			min = val_buf[i];
		}
	}
	return min;
}

void render_gui(Rectangle* rect){
	DrawRectangleLinesEx(*rect, 2, WHITE);
	char upper_label[256];
	char lower_label[256];
	sprintf(upper_label, "%f", get_buf_max());
	sprintf(lower_label, "%f", get_buf_min());
	int fontsize = 32;
	int margin = 8;
	DrawText(upper_label, rect->x+margin, rect->y+margin, fontsize, WHITE);
	DrawText(lower_label, rect->x+margin, rect->y+rect->height-fontsize-margin, fontsize, WHITE);
}

void render_buf(Rectangle* rect){
	render_gui(rect);
	
	int prevx = 0;
	int prevy = 0;

	// index ops to draw last written value first
	size_t i = val_buf_index;
	if(i == 0){
		i = BUFCAP;
	}
	i--;

	int col = buf_size;
	while(col > 0){
		if(i == 0){
			i += BUFCAP;
			i--;
			continue;
		}
		size_t pi = (i+buf_size - 1)%buf_size;
		int y = map(val_buf[i],get_buf_min(),get_buf_max(),rect->height,0)+rect->y;
		int x = map(col,0,buf_size,0,rect->width)+rect->x;
		if(col < buf_size){
			DrawLine(prevx,prevy,x,y,WHITE);
		}	
		prevx = x;
		prevy = y;

		col--;
		i--;
	}
}

typedef struct SPlot_Options SPlot_Options;
struct SPlot_Options{
	char plot_name[256];
	int window_x;
	int window_y;
	int window_w;
	int window_h;
};

void print_help(){
	printf("splot plots input value read from stdin in a graphical window in realtime\n\n");
	printf("usage: splot [options]\n");
	printf("options:\n");
	printf("  -x/y/h/w [value]: sets x, y, height or width (value must be positive)\n");
	printf("  -n [name]: set window name of the plot\n");
	printf("stdin: values to be plotted should be seperated by a newline ('\\n')\n");
}

void parse_options(SPlot_Options* options, int argc, char** argv){
	for(size_t i = 1; i < argc; i++){
		int ret = -1;
		if(argv[i][0] == '-'){
			if(i+1 >= argc){
				goto INVALID_ARGS;
			}
			switch (argv[i][1]) {
				case 'x': {
					i++;
					ret = sscanf(argv[i], "%d", &options->window_x);
				}break;
				case 'y': {
					i++;
					ret = sscanf(argv[i], "%d", &options->window_y);
				}break; 
				case 'w': {
					i++;
					ret = sscanf(argv[i], "%d", &options->window_w);
				}break;
				case 'h': {
					i++;
					ret = sscanf(argv[i], "%d", &options->window_h);
				}break;
				case 'n': {
					i++;
					strcpy(options->plot_name, argv[i]);
					continue;
				}break;
				default:
			}
			if(ret < 0){
INVALID_ARGS:			print_help();
				exit(1);
			}
		}else{
			return;
		}
	}
}

int main(int argc, char** argv){
	SPlot_Options options ={
		.plot_name = "SPlot",
		.window_x = 0,
		.window_y = 0,
		.window_w = 800,
		.window_h = 600
	};
	parse_options(&options, argc, argv);
	
	printf("%d,%d,%d,%d\n",options.window_x,options.window_y,options.window_w,options.window_h);
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(options.window_w, options.window_h, options.plot_name);
	SetWindowPosition(options.window_x,options.window_y);
	Vector2 pos = GetWindowPosition();
	printf("%f,%f\n",pos.x,pos.y);
	
	Rectangle rect = { 20,20,options.window_w-40,options.window_h-40};

	while(!WindowShouldClose()){
		rect.height = GetScreenHeight() - 40;
		rect.width = GetScreenWidth() - 40;
		read_stream(stdin);
		BeginDrawing();
		ClearBackground(BLACK);
		render_buf(&rect);
		EndDrawing();
	}

}
