#define RINGBUF_IMPLEMENTATION
#include "ringbuf.h"

#include <raylib.h>

#define TARGET_FPS 60.0f
#define FRAME_DURATION (1.0f/TARGET_FPS)
double next_frame_time = 0.0f;

static RingBuf data_buffer;

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


ValType parse_val(){
	ValType val;
	int res = sscanf(str_buf,"%f\n",&val);
	if(!res){ exit(1); }
	return val;
}

void read_stream(FILE* stream){
	next_frame_time = GetTime() + FRAME_DURATION;
	char c = 0;
	size_t str_i = 0;
	while(c != EOF && str_i < STRBUFCAP){
		c = fgetc(stream);
		if(c == '\n'){
			ValType val = parse_val();
			RingBuf_write(&data_buffer,val);
			str_i = 0;
			str_buf[str_i] = '\0';
			if(GetTime() > next_frame_time){
				return;
			}
			
		}else if(c != EOF){
			str_buf[str_i] = c;
			str_i++;
			str_buf[str_i] = '\0';
		}
	}
}

void render_gui(Rectangle* rect){
	DrawRectangleLinesEx(*rect, 2, WHITE);
	char upper_label[256];
	char lower_label[256];
	sprintf(upper_label, "%f", RingBuf_get_max(&data_buffer));
	sprintf(lower_label, "%f", RingBuf_get_min(&data_buffer));
	int fontsize = 32;
	int margin = 8;
	DrawText(upper_label, rect->x+margin, rect->y+margin, fontsize, WHITE);
	DrawText(lower_label, rect->x+margin, rect->y+rect->height-fontsize-margin, fontsize, WHITE);
}

float zoom_factor = 0.1f;
float zoom = 0.0f;
void render_buf(Rectangle* rect){
	render_gui(rect);
	
	zoom += GetMouseWheelMove();
	if(zoom < 0){ zoom = 0.0f; }
	
	int buf_window_size = data_buffer.size-1;
	for(size_t i = 0; i < zoom; i++){
		buf_window_size = buf_window_size - (buf_window_size * zoom_factor);
	}

	// index ops to draw last written value first
	
	int buf_window_start = data_buffer.size - buf_window_size;

	ValType* head = data_buffer.write_head;
	int col = data_buffer.size-1;
	ValType min = RingBuf_get_min(&data_buffer);
	ValType max = RingBuf_get_max(&data_buffer);
	int prevy = map(*head,min,max,rect->height,0)+rect->y;
	int prevx = map(col,buf_window_start,data_buffer.size,0,rect->width)+rect->x;
	//printf("draw range: %f - %d\n, draw start: %d,%d",zoom,col,prevx,prevy);
	while(col >= buf_window_start){

		int y = map(*head,min,max,rect->height,0)+rect->y;
		int x = map(col,buf_window_start,data_buffer.size,0,rect->width)+rect->x;
		DrawLine(prevx,prevy,x,y,WHITE);
		prevx = x;
		prevy = y;

		RingBuf_retreat_head(&data_buffer, &head);
		col--;
	}
	//printf("%f-%f\n",min,max);
}

typedef struct SPlot_Options SPlot_Options;
struct SPlot_Options{
	char plot_name[256];
	int window_x;
	int window_y;
	int window_w;
	int window_h;
	size_t initial_capacity;
};

void print_help(){
	printf("splot plots input value read from stdin in a graphical window in realtime\n\n");
	printf("usage: splot [options]\n");
	printf("options:\n");
	printf("  -x/y/h/w [value]	: sets x, y, height or width (value must be positive)\n");
	printf("  -n [name]		: set window name of the plot\n");
	printf("  -c [amount]		: set initial buffer capacity (n values)\n");
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
				case 'c': {
					i++;
					ret = sscanf(argv[i], "%lu", &options->initial_capacity);
					continue;
				}break;

				default: break;
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
		.window_h = 600,
		.initial_capacity = 1024 
	};
	parse_options(&options, argc, argv);
	
	//printf("%d,%d,%d,%d\n",options.window_x,options.window_y,options.window_w,options.window_h);
	
	RingBuf_init(&data_buffer,options.initial_capacity);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(options.window_w, options.window_h, options.plot_name);
	SetTargetFPS(TARGET_FPS);
	SetWindowPosition(options.window_x,options.window_y);
	Vector2 pos = GetWindowPosition();
	//printf("%f,%f\n",pos.x,pos.y);
	
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
