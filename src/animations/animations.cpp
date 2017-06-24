#include <serial/serial_control.cpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/rapidjson.h>
using namespace rapidjson;
#include <iostream>
#include <fstream>

std::string xyjson(decke, int, int);

class ani_frame {

	public:
		ani_frame();
		ani_frame(int, int, char *);
		std::string to_json();
	private:
		decke d;
};

ani_frame::ani_frame(int cx, int cy, char * json){
	d = decke(cx,cy);
	// read defined json frame
}

ani_frame::ani_frame(){
	//empty
	d = decke(2,2);
}

std::string ani_frame::to_json(){
	xyjson(d,d.getx(),d.gety());
}




class ani_ctx {

	public:
		ani_ctx(std::string,int,int,int,int);
		ani_ctx(std::string);
		void add_frame(char *);
		std::string id;
		int sizex;
		int sizey;
		int frame_count;
		int fps;
		std::vector<ani_frame> frames;
		std::string to_string();
};

ani_ctx load_animation(std::string);


ani_ctx::ani_ctx(std::string name, int x, int y, int cf, int cfps){
	sizex = x;
	sizex = y;
	frame_count = cf;
	fps = cfps;
	id = name;
}

ani_ctx::ani_ctx(std::string path){
	sizex = 0;
	sizey = 0;
	frame_count = 0;
	fps = 0;
	id = "";

}

void ani_ctx::add_frame(char * json){
	frames.push_back(ani_frame(sizex, sizey, json));
}

std::string ani_ctx::to_string(){
	StringBuffer s;
    Writer<StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("ctx");

    writer.StartObject();
    writer.Key("id");
    writer.String(id.c_str());
    writer.Key("x");
    writer.Uint(sizex);
    writer.Key("y");
    writer.Uint(sizey);
    writer.Key("frames");
    writer.Uint(frame_count);
    writer.Key("fps");
    writer.Uint(fps);
    writer.EndObject();

    writer.Key("frames");
    writer.StartArray();
    for ( auto f : frames)
    	writer.String(f.to_json().c_str());
    return s.GetString();
}

/*
std::string ani_ctx::to_file(){

	}
*/

void save_animation(std::string path, ani_ctx a){
	//write to file
	std::ofstream file;
  	file.open (path);
  	file << a.to_string();
  	file.close();
}

ani_ctx load_animation(std::string path){
	std::ifstream file;
	file.open(path);

}

void play_animation(std::string name){
	//play directly via serial;
}

uint64_t rgbwtostr(rgbw rgb){
	char arr[8],n,i=0;
	char * t = ((char *)&rgb) - 1; //Eris hates me now
	while ( i<8){
		if ( i%2 ){
			t++;
			n = (*t && 0xF);
		}else
			n = (*t >> 2);
		if ( n < 10)
			arr[i] = n + 48;
		else
			arr[i] = n + 55;
		i++;
	}
	return (uint64_t)*arr;
}

std::string xyjson(decke d,int x, int y){
	
    StringBuffer s;
    Writer<StringBuffer> writer(s);
    writer.StartArray();

    for ( int i = 0; i < x ; i++){
    	for (int j = 0; j < y; j++){
    		writer.StartObject();
    		writer.Key("x");
    		writer.Uint(i);
    		writer.Key("y");
    		writer.Uint(j);
    		writer.Key("RGBW");
    		writer.Key((char *)rgbwtostr(d.getPixel(i,j)));
    		writer.EndObject();
    	}
    }

    writer.EndArray();
/*
    writer.StartObject();               // Between StartObject()/EndObject(), 
    writer.Key("hello");                // output a key,
    writer.String("world");             // follow by a value.
    writer.Key("t");
    writer.Bool(true);
    writer.Key("f");
    writer.Bool(false);
    writer.Key("n");
    writer.Null();
    writer.Key("i");
    writer.Uint(123);
    writer.Key("pi");
    writer.Double(3.1416);
    writer.Key("a");
    writer.StartArray();                // Between StartArray()/EndArray(),
    for (unsigned i = 0; i < 4; i++)
        writer.Uint(i);                 // all values are elements of the array.
    writer.EndArray();
    writer.EndObject();
*/
    // {"hello":"world","t":true,"f":false,"n":null,"i":123,"pi":3.1416,"a":[0,1,2,3]}
    //std::cout << s.GetString() << "\n";
    return s.GetString();

}

