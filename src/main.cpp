/**
Rocket Launch simulation project
Solo 
Hadi EL HAJJ
*/


#include "vcl/vcl.hpp"
#include <iostream>
#include <math.h> 
#include <algorithm>    // std::max

using namespace vcl;

// ****************************************** //
// Structures specific to the rocket scene
// ****************************************** //

struct user_interaction_parameters {
	vec2 mouse_prev;
	bool cursor_on_gui;
	bool display_frame = false;
};
user_interaction_parameters user; // Variable used to store user interaction and GUI parameter

struct scene_environment
{
	camera_around_center camera;
	mat4 projection;
	vec3 light;
};
scene_environment scene; // Generic elements of the scene (camera, light, etc.)

// Structure for the rocket which contains properties of the rocket (position and velocity)
struct rocket_structure
{
	vec3 p0;  // Initial position
	vec3 v0;  // Initial velocity
};

// First stage of the rocket
struct first_Stage
{
	vec3 p0;  // Initial position
	vec3 v0;  // Initial velocity
	vec3 p;   // Position at time t 
	vec3 v;   // Velocity at time t
};

// Second stage of the rocket 
struct second_stage
{
	vec3 p0;  // Initial position
	vec3 v0;  // Initial velocity
	vec3 p;   // Position at time t 
	vec3 v;   // Velocity at time t
};

// Stage carrying the payload of the rocket 
struct payload_fairing
{
	vec3 p0;  // Initial position
	vec3 v0;  // Initial velocity
	vec3 p;   // Position at time t 
	vec3 v;   // Velocity at time t
};


// Satellite, payload of the rocket
struct satellite
{
	vec3 p;   // Position at time t 
	vec3 v;   // Velocity at time t
};


// ****************************************** //
// Functions signatures
// ****************************************** //

// Callback functions
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void window_size_callback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);


void initialize_data(); // Initialize the data of this scene
void display_scene(float current_time);   


// ****************************************** //
// Global variables
// ****************************************** //

//the rocket visualised in the scene 
rocket_structure rocket;    

//three parts of the rocket
first_Stage firstStage;     
second_stage secondStage;
payload_fairing payloadFairing;

//the payload
satellite satellite; 

//meshes representing objects in the scene
mesh_drawable ground;     // Visual representation of the ground

mesh_drawable water;     //water surrounding launch pad

mesh_drawable launch_space;  // Launch space

mesh_drawable road;     //road

mesh_drawable rocket_position_marker; // balck disc used to represent initial launch position of the rocket

mesh_drawable rocket_first_stage;  // first stage body part of the rocket

mesh_drawable rocket_second_stage; // second stage body part of the rocket 

mesh_drawable rocket_payload_fairing;  // body part of the rocket transporting the payload to be sent into space

mesh_drawable satellite_mesh;   // mesh representing the satellite 

mesh_drawable launch_complex; //building positioned next to rocket on launch pad

mesh_drawable global_frame;  // Frame used to see the global coordinate system

// Towers on the corners of the pad (Faraday Cage)
mesh_drawable tower_1; 
mesh_drawable tower_2; 
mesh_drawable tower_3;
mesh_drawable tower_4;

// Thrust 
mesh_drawable thrust;   // display rocket thrust

// Normal direction of the thrust
vec3 thrust_normal = vec3(0,1,0); 

// booleans to check for separations
bool first_stage_separated = false ; 
bool second_stage_separated = false ; 

// bool to lock camera on rocket when L is pressed
bool lock_camera = false ; 

// stage separation times 
float t_separation_first; 
float t_separation_second; 
float t_Satellite_orbit_start; 

// satellite rotation radius - rotation phase around the earth
bool radius_set = false ; 
float angle_of_rotation = 0.0f ; 
float radius ; 

// drawable depicting earth for the orbit phase
mesh_drawable earth ; 
float earth_radius = 1000 ; 

timer_event_periodic timer(0.01f);  // Timer with periodic event

// ****************************************** //
// Functions definitions
// ****************************************** //

// Main function with creation of the scene and animation loop
int main(int, char* argv[])
{
	std::cout << "Run " << argv[0] << std::endl;

	// create GLFW window and initialize OpenGL
	GLFWwindow* window = create_window(1280,1024); 
	window_size_callback(window, 1280, 1024);
	std::cout << opengl_info_display() << std::endl;;

	imgui_init(window); // Initialize GUI library

	// Set GLFW callback functions
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouse_move_callback); 
	glfwSetWindowSizeCallback(window, window_size_callback);
	
	std::cout<<"Initialize data ..."<<std::endl;
	initialize_data();

	std::cout<<"Start animation loop ..."<<std::endl;
	timer.start();
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		scene.light = scene.camera.position();
		timer.update();

		// Clear screen

		// Start with a blue sky color then go all the way to black as the rocket gains altitude
		glClearColor(std::max(0.529f-(timer.t/t_Satellite_orbit_start)*0.529f,0.0f), std::max(0.808f-(timer.t/t_Satellite_orbit_start)*0.888f, 0.0f), std::max(0.922f-(timer.t/t_Satellite_orbit_start)*0.922f,0.0f), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Create GUI interface for the current frame
		imgui_create_frame();
		ImGui::Begin("GUI",NULL,ImGuiWindowFlags_AlwaysAutoResize);
		user.cursor_on_gui = ImGui::IsAnyWindowFocused();
		ImGui::Checkbox("Display frame", &user.display_frame);
		ImGui::SliderFloat("Time Scale", &timer.scale, 0.0f, 3.0f, "%.1f");


		//display_scene();
		if(user.display_frame)
			draw(global_frame, scene);
		
		// ****************************************** //
		// Specific calls of this scene
		// ****************************************** //

		// Display the scene (includes the computation of the particle position at current time)
		display_scene(timer.t);

		// ****************************************** //
		// ****************************************** //

		// Display GUI
		ImGui::End();
		imgui_render_frame(window);

		// Swap buffer and handle GLFW events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	imgui_cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void initialize_data()
{
	// Render the billboard well
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load and set the common shaders
	GLuint const shader_mesh = opengl_create_shader_program(opengl_shader_preset("mesh_vertex"), opengl_shader_preset("mesh_fragment"));
	GLuint const shader_single_color = opengl_create_shader_program(opengl_shader_preset("single_color_vertex"), opengl_shader_preset("single_color_fragment"));
	GLuint const texture_white = opengl_texture_to_gpu(image_raw{1,1,image_color_type::rgba,{255,255,255,255}});

	mesh_drawable::default_shader = shader_mesh;
	mesh_drawable::default_texture = texture_white;
	curve_drawable::default_shader = shader_single_color;
	global_frame = mesh_drawable(mesh_primitive_frame());
	scene.camera.look_at({10,15,10}, {0,0,0}, {0,0,1});

	// prepare the ground mesh
	ground = mesh_drawable(mesh_primitive_quadrangle(vec3(10,10,0),vec3(10,-10,0),vec3(-10,-10,0),vec3(-10,10,0)));
	ground.shading.color = {0.0f,1.f,0.5f}; 

	// water mesh 
	water = mesh_drawable(mesh_primitive_quadrangle(vec3(10,20,-0.01),vec3(10,-20,-0.01),vec3(-20,-20,-0.01),vec3(-20,20,-0.01)));
	water.shading.color = {0.0f, 0.467f, 0.745f};

	// circular space depicting the launch space of the rocket
	launch_space = mesh_drawable(mesh_primitive_disc(5.f,vec3(0,0,0.01),vec3(0,0,1),60));
	launch_space.shading.color = {0.517f,0.407f,0.439f};

	// road for a bit more realistic rendering
	road = mesh_drawable(mesh_primitive_quadrangle(vec3(10,-2.5,0.01),vec3(10,2.5,0.01),vec3(4,2.5,0.01),vec3(4,-2.5,0.01))); 
	road.shading.color = {0.517f,0.407f,0.439f};

	// circular space depicting the launch position of the rocket
	rocket_position_marker = mesh_drawable(mesh_primitive_disc(1.f,vec3(0,0,0.1),vec3(0,0,1),60)); 
	rocket_position_marker.shading.color = {0.0f,0.0f,0.0f}; 
	
	// rocket body on top of the rocket position marker
	rocket_first_stage = mesh_drawable(mesh_primitive_cylinder(0.4f,vec3(0,0,0.001),vec3(0,0,5),60,60,true)); 
	rocket_second_stage = mesh_drawable(mesh_primitive_cylinder(0.4f,vec3(0,0,5.001),vec3(0,0,7),60,60,true)); 
	rocket_second_stage.shading.color = {0.0f,0.0f,0.0f};
	rocket_payload_fairing = mesh_drawable(mesh_primitive_cone(0.6f,1.f,vec3(0,0,7),vec3(0,0,1),true,60,60)); 
	satellite_mesh = mesh_drawable(mesh_primitive_cone(12.f,20.f,vec3(0,0,7),vec3(0,0,1),true,60,60));

	//initialize rocket global data
	rocket.p0 = vec3(0,0,0); 
	rocket.v0 = vec3(0,0,0); 

	//initialize body parts data 
	firstStage.p0 = vec3(0,0,0); 
	firstStage.v0 = vec3(0,0,0); 

	secondStage.p0 = vec3(0,0,0); 
	secondStage.v0 = vec3(0,0,0); 

	payloadFairing.p0 = vec3(0,0,0); 
	payloadFairing.v0 = vec3(0,0,0); 

	//prepare the pad infrastructure (buildings and towers around the rocket)
	launch_complex = mesh_drawable(mesh_primitive_cuboid(vec3(-2.f,0,0),2.f,7.f));

	//prepare towers
	tower_1 = mesh_drawable(mesh_primitive_pentahedron(vec3(-10,-10,0),vec3(-9.5,-10,0),vec3(-9.5,-9.5,0),vec3(-10,-9.5,0),vec3(-9.75,-9.75,10)));
	tower_2 = tower_1 ; 
	tower_2.transform.translate = {0,19.5f,0}; 
	tower_3 = tower_1 ; 
	tower_3.transform.translate = {19.5f,0,0}; 
	tower_4 = tower_1 ; 
	tower_4.transform.translate = {19.5f,19.5f,0}; 

	//rocket thrust billboard

	//import the image
	GLuint const texture_billboard = opengl_texture_to_gpu(image_load_png("assets/thrust.png"));
	float const L = 0.8f; //size of the quad depicting the billboard
	thrust = mesh_drawable(mesh_primitive_quadrangle({0,-1.5L,-2L},{0,1.5L,-2L},{0,1.5L,0},{0,-1.5L,0}));
	thrust.texture = texture_billboard;

	//set stage separation times 
	t_separation_first = 10 ;
	t_separation_second = 15 ; 

	//set time to start satellite orbit phase
	t_Satellite_orbit_start = 20 ; 

	// earth

	//import earth texture image
	GLuint const earthtexture = opengl_texture_to_gpu(image_load_png("assets/earth.png"));
	earth =  mesh_drawable(mesh_primitive_sphere(earth_radius, vec3{0,0,-earth_radius}, 120, 60));
	earth.texture = earthtexture ; 
}


void display_scene(float current_time)
{
	vec3 const g = {0,0,9.81f}; // gravity constant

	//rocket 
	vec3 p = vec3(0,0,0) ;
	vec3 v = vec3(0,0,0) ; 

	// MAKE TRAJECTORY CALCULATIONS

	//rocket 

	/* simple trajectory and velocity. In a more realistic simulation, these parameters should be 
	replaced by equations depicting rocket propulsion */ 
	v = vec3(0,0,5.f) ; 
	p = rocket.p0 + vec3(0,0,v.z*current_time); 


	if(current_time < t_Satellite_orbit_start){   // pre satellite orbit stage, i.e. rocket launch
		//first stage 
		if(current_time >= t_separation_first && !first_stage_separated ){  //handle separation 
			//reinitialize parameters to use in separation phase
			firstStage.v0  = firstStage.v ; 
			firstStage.p0 = firstStage.p ;
			first_stage_separated = true ; 
		}
		else{ 
			if(first_stage_separated){
				//separation stage dynamics (gravity pull)
				if(firstStage.p.z>0){   
					//simple collision with floor detection  
					firstStage.v = -g*(current_time-t_separation_first) + firstStage.v0; 
					firstStage.p = -0.5*g*(current_time-t_separation_first)*(current_time-t_separation_first) + firstStage.v0*(current_time-t_separation_first) + firstStage.p0; 
				}
				else{  
					//if collision set position (position correction for collision detection and handling)
					firstStage.v = vec3(0,0,0);  
					firstStage.p = vec3(0,0,0); 
				}
			}
			else{
				//pre-separation stage dynamics 
				firstStage.v = v ; //the velocity of the first stage is equal to the velocity of the rocket
				firstStage.p = firstStage.p0 + v*current_time ;
			}
		}

		// second stage - same exact steps as for the first stage
		if(current_time >= t_separation_second && !second_stage_separated ){
			secondStage.v0  = secondStage.v ; 
			secondStage.p0 = secondStage.p ;
			second_stage_separated = true ; 
		}
		else{ 
			if(second_stage_separated){
				if(secondStage.p.z>0){   //simple collision with floor detection  
					secondStage.v = -g*(current_time-t_separation_second) + secondStage.v0; 
					secondStage.p = -0.5*g*(current_time-t_separation_second)*(current_time-t_separation_second) + secondStage.v0*(current_time-t_separation_second) + secondStage.p0; 
				}
				else{
					secondStage.v = vec3(0,0,0); 
					secondStage.p = vec3(0,0,0); 
				}
			}
			else{
				secondStage.v = v ; //the velocity of the first stage is equal to the velocity of the rocket
				secondStage.p = secondStage.p0 + v*current_time ; 
			}
		}

		/*payload fairing - no separating. For the sake of simplification, the payload fairing is taken here
		as the payload itself*/  
		payloadFairing.v = v ; //the velocity of the first stage is equal to the velocity of the rocket
		payloadFairing.p = payloadFairing.p0 + v*current_time ; 

		//APPLY TRANSFORMS - translate to the new positions
		rocket_first_stage.transform.translate={firstStage.p.x,firstStage.p.y,firstStage.p.z}; 
		rocket_second_stage.transform.translate={secondStage.p.x,secondStage.p.y,secondStage.p.z}; 
		rocket_payload_fairing.transform.translate={payloadFairing.p.x,payloadFairing.p.y,payloadFairing.p.z};

		// /* translation and rotation of the billboard
		// 	- the translation follows the rocket, so it is a translation upwards (along z-axis)
		// 	  with the same velocity of the rocket
		// 	- the rotation should not be done according to camera's orientation, but should always 
		// 	  happen along the vertical axis of the rocket, in this case the chosen axis z */

		/* translation */ 
		if(!first_stage_separated){
			thrust.transform.translate = {firstStage.p.x,firstStage.p.y,firstStage.p.z} ; //thrust follows first stage 
		}else if(first_stage_separated && !second_stage_separated){
			thrust.transform.translate = {secondStage.p.x,secondStage.p.y,secondStage.p.z+5} ; //thrust is now coming from second stage 
		}else if(first_stage_separated && second_stage_separated){
			thrust.transform.translate = {payloadFairing.p.x,payloadFairing.p.y,payloadFairing.p.z+7} ; //thrust is now coming from payload fairing
		}

		/* rotation */ 
		//no attempts were made for the rotation part


		if(lock_camera){  // if L button is pressed lock camera in animation view
			//liftoff
			scene.camera.look_at({10,15,10}, p , {0,0,1}) ; 
			
			//rocket soaring through the sky 
			if (current_time >=6){
				//follow the rocket on the z axis
				scene.camera.look_at({10,15,p.z}, p , {0,0,1}); 
			}
		}

		//DISPLAY ELEMENTS

		// Display the ground
		draw(ground, scene);
		draw(water, scene); 
		draw(launch_space,scene); 
		draw(road,scene); 
		// Display the rocket's initial position marker
		draw(rocket_position_marker,scene); 
		// Display the rocket by displaying its parts one by one
		draw(rocket_first_stage,scene); 
		draw(rocket_second_stage,scene);
		draw(rocket_payload_fairing,scene);
		// Display the launch pad complex
		draw(launch_complex, scene);
		// Display towers
		draw(tower_1,scene);  
		draw(tower_2,scene);  
		draw(tower_3,scene);  
		draw(tower_4,scene);  
		// Display thrust billboard
		glDepthMask(false); 
		draw(thrust,scene); 
		glDepthMask(true); 
		draw(earth, scene); 
	}
	else{   //satellite orbit phase 

		if(!radius_set){
			//set radius once only
			//radius of rotation 
			radius = payloadFairing.p.z + earth_radius; 
			radius_set = true; 
		}

		//calulate trajectory/rotation 
		/* 
		having taken the center of the earth at (0,0,-earth_radius), the rotation matrix applied should be
		one  of a rotation around an arbitrary point in space as seen in class, here the arbitrary point being 
		(0,0,-earth_radius) 
		*/ 

		//angle 
		angle_of_rotation = 0.5*(current_time - t_Satellite_orbit_start); 

		/* 
		here the rotation is coded manually. Here are some explanations: 
			-rotation is done around the y axis, which explains why the y component of the position stays the same
			-to render a rotation around the point c_earth = (0,0,-earth_radius), we first have to translate the satellite center of rotation to
			 the center of the world space, thus translating the point c_earth from its original position to the origin
			-then we code the rotation, which is given here by (Rsin(theta), y, Rcos(theta)) 
			-last, we translate again to go back to the original center of rotation, which explains the -earth_radius on the z component
			-the -7 is taken into account to take out the offset of 7 on the z axis defined when the mesh was first configured on line 284 :

			 satellite_mesh = mesh_drawable(mesh_primitive_cone(12.f,20.f,vec3(0,0,7),vec3(0,0,1),true,60,60));
			 																	   ^  
		*/
		satellite.p = vec3(radius*std::sin(angle_of_rotation),satellite.p.y,radius*std::cos(angle_of_rotation)-7 -earth_radius); 
		// do the transform 
		satellite_mesh.transform.translate= satellite.p ; 

		if(lock_camera){
			// give the camera a steady position on the y axis for better global view
			scene.camera.look_at({0,-3000,-earth_radius}, {0,0,-earth_radius} , {0,0,1}); 
		}

		//DISPLAY ELEMENTS

		// Display the ground
		draw(ground, scene);
		draw(water, scene); 
		draw(launch_space,scene); 
		draw(road,scene); 
		// Display the rocket's initial position marker
		draw(rocket_position_marker,scene); 
		// Display the rocket by displaying its parts one by one
		draw(rocket_first_stage,scene); 
		draw(rocket_second_stage,scene);
		draw(satellite_mesh,scene);
		// Display the launch pad complex
		draw(launch_complex, scene);
		// Display towers
		draw(tower_1,scene);  
		draw(tower_2,scene);  
		draw(tower_3,scene);  
		draw(tower_4,scene);  
		draw(earth, scene); 
	}
}

// Function called every time the screen is resized
void window_size_callback(GLFWwindow* , int width, int height)
{
	glViewport(0, 0, width, height);
	float const aspect = width / static_cast<float>(height);

	//modifying the far so we can still see the rocket from afar while moving the camera position
	scene.projection = projection_perspective(50.0f*pi/180.0f, aspect, 0.1f, 10000000000.0f);
}


// Function called every time the mouse is moved
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
	vec2 const  p1 = glfw_get_mouse_cursor(window, xpos, ypos);
	vec2 const& p0 = user.mouse_prev;

	glfw_state state = glfw_current_state(window);

	// Handle camera rotation
	auto& camera = scene.camera;
	if(!user.cursor_on_gui){
		if(state.mouse_click_left && !state.key_ctrl)
			scene.camera.manipulator_rotate_trackball(p0, p1);
		if(state.mouse_click_left && state.key_ctrl)
			//added a factor to have a faster control over the camera
			camera.manipulator_translate_in_plane(10*(p1-p0));
		if(state.mouse_click_right)
			camera.manipulator_scale_distance_to_center( (p1-p0).y );
	}
	user.mouse_prev = p1;
}

// Function called every time a key is entered.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(action == GLFW_REPEAT && key == GLFW_KEY_1) {
		scene.camera.look_at({10,15,firstStage.p.z}, firstStage.p , {0,0,1});
	} else if(action == GLFW_REPEAT && key == GLFW_KEY_2) {
		scene.camera.look_at({10,15,secondStage.p.z}, secondStage.p , {0,0,1});
	} else if(action == GLFW_REPEAT && key == GLFW_KEY_3){
		scene.camera.look_at({10,15,payloadFairing.p.z}, payloadFairing.p , {0,0,1});
	} else if(action == GLFW_REPEAT && key == GLFW_KEY_4){
		if(satellite.p.x >0 ){
			if(satellite.p.z> -earth_radius){
				scene.camera.look_at({satellite.p.x+ 60,15,satellite.p.z+60}, satellite.p , {0,0,1});
			}
			else{
				scene.camera.look_at({satellite.p.x+ 60,15,satellite.p.z-60}, satellite.p , {0,0,1});
			}
		}
		else{
			if(satellite.p.z> -earth_radius){
				scene.camera.look_at({satellite.p.x- 60,15,satellite.p.z+60}, satellite.p , {0,0,1});
			}
			else{
				scene.camera.look_at({satellite.p.x- 60,15,satellite.p.z-60}, satellite.p , {0,0,1});
			}
		}
	} else if(action == GLFW_PRESS && key == GLFW_KEY_B){
		scene.camera.look_at({10,15,0}, {0,0,payloadFairing.p.z} , {0,0,1});
	} else if(action == GLFW_PRESS && key == GLFW_KEY_L){
		lock_camera = !lock_camera ; 
	}else if(action == GLFW_PRESS && (key == GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, true); // Closes the application if the escape key is pressed
	}
}

// Uniform data used when displaying an object in this scene
void opengl_uniform(GLuint shader, scene_environment const& current_scene)
{
	opengl_uniform(shader, "projection", current_scene.projection);
	opengl_uniform(shader, "view", scene.camera.matrix_view());
	opengl_uniform(shader, "light", scene.light, false);
}



