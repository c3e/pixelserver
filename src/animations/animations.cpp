#include <serial/serial_control.cpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/schema.h>

using namespace rapidjson;
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sstream>
#include <iomanip>

std::string xyjson(decke, int, int);
uint64_t rgbwtostr(rgbw);
rgbw strtorgbw(uint64_t);




class ani_frame {

	public:
		ani_frame();
		ani_frame(int, int);
        ani_frame(decke &);
        std::string to_json();
	    decke d;
};

ani_frame::ani_frame(int cx, int cy){
	d = decke(cx,cy);
	// read defined json frame
}

ani_frame::ani_frame(decke &de){
    //empty
    d = de;
}

ani_frame::ani_frame(){
	//empty
	d = decke(2,2);
}

std::string ani_frame::to_json(){
	return xyjson(d,d.getx(),d.gety());
}




class ani_ctx {

	public:
		ani_ctx(std::string,int,int,int,int);
		ani_ctx(std::string);
		void add_frame(decke &);
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
    using namespace rapidjson;
    FILE* fp = fopen(path.c_str(), "r"); // Windows use "rb"
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    if ( d.HasMember("ctx") ){
        for (Value::MemberIterator m = d["ctx"].MemberBegin(); m != d["ctx"].MemberEnd(); ++m){
            if (m->name.GetString() == "id")
                id = m->value.GetString();
            else if ( strncmp(m->name.GetString(),"x",1) == 0)
                sizex = m->value.GetInt();
            else if ( strncmp(m->name.GetString(),"y",1) == 0)
                sizey = m->value.GetInt();
            else if ( strncmp(m->name.GetString(),"f",1) == 0)
                frame_count = m->value.GetInt();
            else if ( strncmp(m->name.GetString(),"fps",3) == 0)
                fps = m->value.GetInt();
        }
    }

    frames = std::vector<ani_frame>(frame_count);
    int x,y;
    rgbw r;
    
    if (d.HasMember("frames")) {
        
        for (Value::MemberIterator m = d["frames"].MemberBegin(); m != d["ctx"].MemberEnd(); ++m){
            
            decke ceil = decke ( sizex,sizey);

            for (Value::MemberIterator n = m->value.MemberBegin(); n != m->value.MemberEnd(); ++n){

                x = atoi(n->value["x"].GetString());
                y = atoi(n->value["y"].GetString());
                std::stringstream(n->value["y"].GetString()) >> std::hex >> r;
                ceil.setPixel(x,y,r);
            }

            add_frame(ceil);

        }    
    }


    fclose(fp);

}

void ani_ctx::add_frame(decke &d){
	frames.push_back(ani_frame(d));
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
    writer.Key("f");
    writer.Uint(frame_count);
    writer.Key("fps");
    writer.Uint(fps);
    writer.EndObject();

    writer.Key("frames");
    writer.StartArray();
    for ( auto f : frames)
    	writer.RawValue(f.to_json().c_str(), f.to_json().length(), kObjectType);
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

rgbw strtorgbw (uint64_t foo){

}

std::string xyjson(decke d,int x, int y){
	
    StringBuffer s;
    Writer<StringBuffer> writer(s);
    writer.StartArray();

    for ( int i = 0; i < x ; i++){
    	for (int j = 0; j < y; j++){
            if ( d.getPixel(i,j) != 0){
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
    }

    writer.EndArray();
    return s.GetString();

}

