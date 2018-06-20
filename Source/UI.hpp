#ifndef UI_hpp
#define UI_hpp

#include "../Externals/Include/Include.h"
#include <algorithm>
#include <ctime>
#include "load_utils.hpp"
using namespace std;

class UI {
public:
	UI();
	void draw();

	TextureData loadPNG(const char* const pngFilepath);
	TextureData tdata_rain;
	TextureData tdata_sun;
	TextureData tdata_select;
	GLuint tex_rain;
	GLuint tex_sun;
	GLuint tex_select;

	bool click_sun(int x, int y);
	bool click_rain(int x, int y);
	void set_mode(int newmode) {
		mode = newmode;
	}
	int get_mode() {
		return mode;
	}
private:
	int mode = 0;
};
#endif