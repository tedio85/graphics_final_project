#include "../Externals/Include/Include.h"
#include "load_utils.hpp"
#include "frame_buffer.hpp"
#include "depth_fbo.h"
#include "fps_counter.h"
#include "light.h"
#include <ctime>

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

using namespace glm;
using namespace std;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

// debug
int cnt = 0;


GLuint window_vao, window_buffer;
GLuint program_debug;
float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
						 // positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	1.0f, -1.0f,  1.0f, 0.0f,
	1.0f,  1.0f,  1.0f, 1.0f
};

// program
GLuint program_shadow;
GLuint program_window;

// model view matrices
GLuint um4p;
GLuint um4mv;
mat4 viewMat;
mat4 projMat;
mat4 modelMat = mat4();

// viewing positions for different models
const vec3 pos = vec3(10.0f, 90.0f, 10.0f);
const vec3 front = vec3(-1.0f, -1.0f, -1.0f);

// camera
vec3 cameraPos = pos;
vec3 cameraFront = front;
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
float cameraSpd = 0.05f * timer_speed;
float pitchDeg = -30.0f;
float yawDeg = 0.0f;

// mouse control
bool firstMouse = true;
int lastX, lastY;

// load model
Model *model;
char modelDir[] = "../Assets/city_block/";
char modelFile[] = "city_block.obj";

// load sun
Model *planet;
char planetDir[] = "../Assets/";
char planetFile[] = "Sphere.obj";

// lighting
Light light;

// shadow mapping
GLuint depth_mvp;
GLuint depth_fbo, depth_tex;
extern const unsigned int SHADOW_WIDTH, SHADOW_HEIGHT;
float near_plane = 0.0f;
float far_plane = 100.0f;
mat4 lightProj = ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);	// projection matrix of light
GLuint shadow_matrix;
mat4 shadowMat;


// fps counter
FPS_Counter *counter;


// function declarations
void createProgram(GLuint &program, char *vs_path, char *fs_path);

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// taken from http://www.codersource.net/2011/01/27/displaying-text-opengl-tutorial-5/
void drawBitmapText(const char *string, float x, float y, float z)
{
	const char *c;
	glRasterPos3f(x, y, z);

	for (c = string; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
	}
}

void My_Init()
{
    glClearColor(0.843f, 0.913f, 0.988f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// enable face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	// turn on MSAA for anti-aliasing
	glEnable(GL_MULTISAMPLE);
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

	// detect current settings of MSAA
	GLint iMultiSample = 0;
	GLint iNumSamples = 0;
	glGetIntegerv(GL_SAMPLE_BUFFERS, &iMultiSample);
	glGetIntegerv(GL_SAMPLES, &iNumSamples);
	printf("MSAA on, GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d\n", iMultiSample, iNumSamples);
    
	// for shadow mapping
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);

	// initialize timer
	counter = new FPS_Counter();


    // load shaders and program
	char vs_depth_path[] = "vertex_depth.vs.glsl";
	char fs_depth_path[] = "fragment_depth.fs.glsl";
	createProgram(program_shadow, vs_depth_path, fs_depth_path);

	char vs_path[] = "vertex.vs.glsl";
	char fs_path[] = "fragment.fs.glsl";
	createProgram(program_window, vs_path, fs_path);

	char vs_path2[] = "vertex2.vs.glsl";
	char fs_path2[] = "fragment2.fs.glsl";
	createProgram(program_debug, vs_path2, fs_path2);
	


    // get uniform location
    um4p = glGetUniformLocation(program_window, "um4p");
    um4mv = glGetUniformLocation(program_window, "um4mv");
	shadow_matrix = glGetUniformLocation(program_window, "shadow_matrix");
	depth_mvp = glGetUniformLocation(program_shadow, "mvp");


    // load model
    model = new Model(modelDir, modelFile);
	planet = new Model(planetDir, planetFile);
	planet -> add_texture(0, "8k_sun.jpg");
    

    // configure lighting
	light.useDefaultSettings();
	light.getUniformLocations(program_window);
   
	// configure variables for shadow mapping
	shadowMat = translate(mat4(), vec3(0.5f));
	shadowMat = scale(shadowMat, vec3(0.5f));
	configure_depth_fbo();


	/////////////////////////////////////////////////
	// prepare vao & vbo for window
	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

// create a new shader program with specified vertex shader path
// and fragment shader path
void createProgram(GLuint &program, char *vs_path, char *fs_path) {
	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource(vs_path);
	char** fragmentShaderSource = loadShaderSource(fs_path);
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
}

// recalculate the view matrix
void refreshView()
{
    // camera
    pitchDeg = clamp(pitchDeg, -89.0f, 89.0f);  // restrict pitch degrees
    cameraFront.x = cos(radians(pitchDeg)) * cos(radians(yawDeg));
    cameraFront.y = sin(radians(pitchDeg));
    cameraFront.z = cos(radians(pitchDeg)) * sin(radians(yawDeg));
    cameraFront = normalize(cameraFront);
    
    
    viewMat = lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
    printf("cameraFront now at (%f, %f, %f)\n",
           cameraFront.x, cameraFront.y, cameraFront.z);
    printf("cameraPos now at (%f, %f, %f)\n",
           cameraPos.x, cameraPos.y, cameraPos.z);
}

void My_Display()
{
	counter->get_start_frequency();

	///////// 1st pass: render depth map from light view ///////////
	// my version
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glUseProgram(program_shadow);

    // set uniforms
	mat4 lightView = lookAt(light.settings.pos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(depth_mvp, 1, GL_FALSE, value_ptr(lightProj * lightView));

	// bind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);

    // render model
	model->render();

	// unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	

	/*
	// TA's version
	vec3 light_position = vec3(20.0f, 20.0f, 20.0f);
	mat4 light_proj_matrix = ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 100.0f); 
	mat4 light_view_matrix = lookAt(light_position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo); 
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); 
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glUseProgram(program_shadow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
	*/
	//model->render();


	////////////////////// visualize //////////////////////////////
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program_debug);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glBindVertexArray(window_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	///////// 2nd pass: render object from camera view ///////////
	/*
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program_window);

	glUniform1i(glGetUniformLocation(program_window, "tex"), 0);
	glUniform1i(glGetUniformLocation(program_window, "shadow_tex"), 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth_tex);

	// set uniforms
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projMat));
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(viewMat * modelMat));
	light.setUniforms();
	glUniformMatrix4fv(shadow_matrix, 1, GL_FALSE, value_ptr(shadowMat));

	// render model
	model->render();
	*/

	// display fps
	counter->get_end_frequency();
	char buffer[100] = {};
	sprintf(buffer, "each call of My_Display() takes %.2e seconds", counter->get_frametime());
	drawBitmapText(buffer, -0.9, 0.95, 0);
	
    glutSwapBuffers();
}


void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
    
    float viewportAspect = (float)width / (float)height;
	projMat = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

    refreshView();
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if(state == GLUT_DOWN)
	{
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if(state == GLUT_UP)
	{
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void myMouseMove(int x, int y)
{
    if(firstMouse){
        lastX = x;
        lastY = y;
        firstMouse = false;
    }
    
    
    float deltaX = (float)(x - lastX);
    float deltaY = (float)(lastY - y);
    
    yawDeg   += deltaX * cameraSpd;
    pitchDeg += deltaY * cameraSpd;
    
    lastX = x;
    lastY = y;
    
    refreshView();
}

void My_Keyboard(unsigned char key, int x, int y)
{
    switch(key){
        case 'w':
            cameraPos += cameraSpd * cameraFront;
            break;
            
        case 's':
            cameraPos -= cameraSpd * cameraFront;
            break;
            
        case 'a':
            cameraPos -= cameraSpd * normalize(cross(cameraFront, cameraUp));
            break;
            
        case 'd':
            cameraPos += cameraSpd * normalize(cross(cameraFront, cameraUp));
            break;
            
        case 'q':
            yawDeg -= cameraSpd;
            break;
            
        case 'e':
            yawDeg += cameraSpd;
            break;
            
        case 'z':
            cameraPos += cameraSpd * cameraUp;
            break;
            
        case 'x':
            cameraPos -= cameraSpd * cameraUp;
            break;
            
        default:
            printf("Key %c is pressed at (%d, %d)\n", key, x, y);
            break;
    }
    refreshView();
}

void My_SpecialKeys(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_F1:
            printf("F1 is pressed at (%d, %d)\n", x, y);
            break;
            
        case GLUT_KEY_PAGE_UP:
            printf("Page up is pressed at (%d, %d)\n", x, y);
            break;
            
        case GLUT_KEY_LEFT:
            printf("Left arrow is pressed at (%d, %d)\n", x, y);
            break;
            
        case GLUT_KEY_RIGHT:
            printf("Right arrow is pressed at (%d, %d)\n", x, y);
            break;
            
        default:
            printf("Other special key is pressed at (%d, %d)\n", x, y);
            break;
    }
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
	glutSetOption(GLUT_MULTISAMPLE, 8);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("final_project"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glewInit(); // added
    glPrintContextInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
    glutPassiveMotionFunc(myMouseMove);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

    
	// Enter main event loop.
	glutMainLoop();

	return 0;
}
