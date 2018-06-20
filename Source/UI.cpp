#include "UI.hpp"
UI::UI() {

	tdata_rain = loadPNG("../Assets/rain.png");
	tdata_sun = loadPNG("../Assets/sun.png");
	tdata_select = loadPNG("../Assets/select.png");
	glGenTextures(1, &tex_rain);
	glBindTexture(GL_TEXTURE_2D, tex_rain);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata_rain.width, tdata_rain.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata_rain.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenTextures(1, &tex_sun);
	glBindTexture(GL_TEXTURE_2D, tex_sun);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata_sun.width, tdata_sun.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata_sun.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenTextures(1, &tex_select);
	glBindTexture(GL_TEXTURE_2D, tex_select);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata_select.width, tdata_select.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata_select.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
}



void UI::draw() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_rain);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2f(0.95, 0.15);
	glTexCoord2f(0, 1); glVertex2f(0.95, 0.05);
	glTexCoord2f(1, 1); glVertex2f(0.85, 0.05);
	glTexCoord2f(1, 0); glVertex2f(0.85, 0.15);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex_sun);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2f(0.80, 0.15);
	glTexCoord2f(0, 1); glVertex2f(0.80, 0.05);
	glTexCoord2f(1, 1); glVertex2f(0.70, 0.05);
	glTexCoord2f(1, 0); glVertex2f(0.70, 0.15);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, tex_select);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

	if (mode == 0) {
		glTexCoord2f(0, 0); glVertex2f(0.80, 0.15);
		glTexCoord2f(0, 1); glVertex2f(0.80, 0.05);
		glTexCoord2f(1, 1); glVertex2f(0.70, 0.05);
		glTexCoord2f(1, 0); glVertex2f(0.70, 0.15);
	}
	else {
		glTexCoord2f(0, 0); glVertex2f(0.95, 0.15);
		glTexCoord2f(0, 1); glVertex2f(0.95, 0.05);
		glTexCoord2f(1, 1); glVertex2f(0.85, 0.05);
		glTexCoord2f(1, 0); glVertex2f(0.85, 0.15);
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
TextureData UI::loadPNG(const char* const pngFilepath) {
	TextureData texture;
	int components;

	// load the texture with stb image, force RGBA (4 components required)
	stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);

	// is the image successfully loaded?
	if (data != NULL)
	{
		printf("loading PNG from %s\n", pngFilepath);

		// copy the raw data
		size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
		texture.data = new unsigned char[dataSize];
		memcpy(texture.data, data, dataSize);

		// mirror the image vertically to comply with OpenGL convention
		for (size_t i = 0; i < texture.width; ++i)
		{
			for (size_t j = 0; j < texture.height / 2; ++j)
			{
				for (size_t k = 0; k < 4; ++k)
				{
					size_t coord1 = (j * texture.width + i) * 4 + k;
					size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
					std::swap(texture.data[coord1], texture.data[coord2]);
				}
			}
		}

		// release the loaded image
		stbi_image_free(data);
	}

	return texture;
}

bool UI::click_sun(int x, int y) {
	if (glutGet(GLUT_INIT_WINDOW_WIDTH)*0.7 <= x
		&& glutGet(GLUT_INIT_WINDOW_WIDTH)*0.8 >= x
		&& glutGet(GLUT_INIT_WINDOW_HEIGHT)*0.05 <= y
		&& glutGet(GLUT_INIT_WINDOW_HEIGHT)*0.15 >= y) {
		set_mode(0);
		return true;
	}
	return false;
}
bool UI::click_rain(int x, int y) {
	if (glutGet(GLUT_INIT_WINDOW_WIDTH)*0.85 <= x
		&& glutGet(GLUT_INIT_WINDOW_WIDTH)*0.95 >= x
		&& glutGet(GLUT_INIT_WINDOW_HEIGHT)*0.05 <= y
		&& glutGet(GLUT_INIT_WINDOW_HEIGHT)*0.15 >= y) {
		set_mode(1);
		return true;
	}
	return false;
}