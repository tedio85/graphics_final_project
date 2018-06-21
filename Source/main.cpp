#include "../Externals/Include/Include.h"
#include "load_utils.hpp"
#include "light.h"
#include "UI.hpp"
#include <ctime>

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define SHADOW_MAP_SIZE 4096

struct
{
	struct
	{
		mat4 view;
		mat4 proj;
	} eye;
} matrices;

struct
{
	int width;
	int height;
} viewportSize;

struct
{
	GLuint fbo;
	GLuint depthMap;
} shadowBuffer;

using namespace glm;
using namespace std;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

// program
GLuint program;
GLuint program_depth;

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

// lighting
Light light;



//UI
UI *ui;
static const GLfloat window_positions[16] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

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
void shaderLog(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		GLchar* errorLog = new GLchar[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		printf("%s\n", errorLog);
		delete errorLog;
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

	// detect current settings
	GLint iMultiSample = 0;
	GLint iNumSamples = 0;
	glGetIntegerv(GL_SAMPLE_BUFFERS, &iMultiSample);
	glGetIntegerv(GL_SAMPLES, &iNumSamples);
	printf("MSAA on, GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d\n", iMultiSample, iNumSamples);
    

    // load shaders and program
    program = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
    char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    freeShaderSource(vertexShaderSource);
    freeShaderSource(fragmentShaderSource);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);

	program_depth = glCreateProgram();
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	vertexShaderSource = loadShaderSource("vertex_depth.vs.glsl");
	fragmentShaderSource = loadShaderSource("fragment_depth.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	glAttachShader(program_depth, vertexShader);
	glAttachShader(program_depth, fragmentShader);
	glLinkProgram(program_depth);
	glUseProgram(program_depth);
    
    
    // get uniform location
    um4p = glGetUniformLocation(program, "um4p");
    um4mv = glGetUniformLocation(program, "um4mv");
    

    // load model
    model = new Model(modelDir, modelFile);

	//UI
	ui = new UI();

    // configure lighting
	light.useDefaultSettings();
	light.getUniformLocations(program);
   

	// configure shadow buffer
	glGenFramebuffers(1, &shadowBuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

	glGenTextures(1, &shadowBuffer.depthMap);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowBuffer.depthMap, 0);

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
    
}

void My_Display()
{
	mat4 scale_bias_matrix =
		translate(mat4(), vec3(0.5f, 0.5f, 0.5f)) *
		scale(mat4(), vec3(0.5f, 0.5f, 0.5f));
	const float shadow_range = 300.0f;
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 5.0f, 1000.0f);
	mat4 light_view_matrix = lookAt(vec3(300.0f, 300.0f, 300.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;

	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

	glUseProgram(program_depth);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glUniformMatrix4fv(glGetUniformLocation(program_depth, "mvp"), 1, GL_FALSE, value_ptr(light_vp_matrix));

	model->render();

	glDisable(GL_POLYGON_OFFSET_FILL);

	//////////////////////////////////////////////////////////////////
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize.width, viewportSize.height);
	glUseProgram(program);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
	glUniform1i(glGetUniformLocation(program, "shadow_tex"), 0);

	// set uniforms
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projMat));
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(viewMat * modelMat));

	light.setUniforms();

	mat4 shadow_matrix = shadow_sbpv_matrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "shadow_matrix"), 1, GL_FALSE, value_ptr(shadow_matrix));

	
    // render model
	model->render();
	


	//UI
	glUseProgram(0);
	ui->draw();

    glutSwapBuffers();
}


void My_Reshape(int width, int height)
{
	viewportSize.width = width;
	viewportSize.height = height;
	glViewport(0, 0, width, height);
    
    float viewportAspect = (float)width / (float)height;
    projMat = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);
    refreshView();


	
	matrices.eye.proj = perspective(radians(50.0f), viewportAspect, 0.1f, 1000.0f);
	matrices.eye.view = lookAt(vec3(0.0f, 0.0f, 40.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
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
		if (ui->click_rain(x, y)) {
			//â∫âJ

		}
		else if (ui->click_sun(x, y)) {
			//ê∞ìV

		}
		
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
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
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
