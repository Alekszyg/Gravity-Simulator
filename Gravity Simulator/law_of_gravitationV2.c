#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <windows.h>

#define FRAME_BUFFER_SIZE 20000

// time units in seconds
#define MINUTE (60)
#define HOUR (MINUTE * 60)
#define DAY (HOUR * 24)
#define WEEK (DAY * 7)

// simulation constants
#define NO_OBJECTS 3
const double GRAVITATIONAL_CONSTANT = 6.67430e-11;
#define M_PI 3.14159265358979323846

// simulation configuration
int delta_time = MINUTE;     // simulaton step duration
int log_step = MINUTE;       // how often data is recorded
int time_scale = (WEEK * 4); // total duration of the simulation

// derived intervals
#define MINUTE_INTERVAL (MINUTE / delta_time)
#define HOUR_INTERVAL (HOUR / delta_time)
#define DAY_INTERVAL (DAY / delta_time)
#define WEEK_INTERVAL (WEEK / delta_time)

// enum for plane axes
enum Planes
{
    XY,
    YZ,
    XZ
};

// render configuration
#define RENDER_SIZE 400000000 // 400 million metres (half-width of view)
// NO_PIXELSX / NO_PIXELSY = 0.81 for square grid
int render_step = DAY;   // how often rendering occurs
bool render_wait = true; // pause after each render
float zoom = 1;          // render zoom level
double view_offsetX = 0;
double view_offsetY = 0;
double view_offsetZ = 0;
double cameraX = 0;
double cameraY = 0;




int view_focused_object = 0;      // what object is the view focused on
int motion_relative_to_object = 0; // what object is the view focused on; // displays motion relative to this object


int plane = XY; //

typedef struct {
    double m[3][3];  // A 3x3 matrix
} Mat3;

typedef struct
{
    double x, y, z;
} Vec3;

typedef struct
{
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
} Motion;

typedef struct
{
    double mass;
    Motion motion;
    char symbol;

} Object;

typedef struct
{
    Vec3 pivot_position;

    double zoom;
    double view_size;
    int no_pixelsX;
    int no_pixelsY;
    double pixel_size_x;
    double pixel_size_y;
    double angular_resolution_x;
    double angular_resolution_y;
} Camera;


#define NO_PIXELSX 32
#define NO_PIXELSY 40



Camera camera = {
    .pivot_position = {0.0f, 0.0f, 0.0f},
    .zoom = 1.0,
    .view_size = 8e8,
    .no_pixelsX = NO_PIXELSX,
    .no_pixelsY = NO_PIXELSY,
};




Vec3 degrees = (Vec3){0, 0, 0};
// x and z verified

// core physics
double distance(Object, Object);
void apply_gravitational_forces(Object *, Object *);
void apply_gravitational_forces_N(Object[]);

// state updates
void update(Object *object);
void update_N(Object[]);

// simulation log
void update_log(Object *, Object[], int time);
Object *get_log_data(Object *sim_log, int time_seconds);

// simulation control
void simulate(Object *sim_log, Object initial_objects[], Object objects[], int time_seconds);

// rendering
void render_objects_static(Object *sim_log, int time_seconds);
char render_interactive(Object *sim_log, int time_seconds, bool have_time_control);
void render_objects_playback(Object *sim_log, int start, int end);
void rotate_render(Object *sim_log, int time_seconds);
Vec3 rotate_z_up(Vec3 v, double spin_deg, double pitch_deg);
void pan_camera(Vec3, double move, double pitch, double yaw);

Vec3 mat3_multiply_vec3(Mat3 mat, Vec3 vec);

// Create a pitch rotation matrix (rotation around X-axis)
Mat3 create_pitch_matrix(double pitch);

// Create a yaw rotation matrix (rotation around Z-axis)
Mat3 create_yaw_matrix(double yaw);








// utility
bool is_interval(int, int);
char *display_time(int);
char *format_number(double number);
double calculate_resolution();
void display_position(Object);
void display_all_information(Object objects[]);
void clear_input_buffer();


// ui
int program_ui(Object *sim_log, Object[], Object[]);
int simulation_ui(Object *sim_log, Object[], Object[]);
int settings_ui();
int simulation_settings_ui();
int render_settings_ui();
void intro();
void menu_banner(int menu);

int main()
{
    Object objects[NO_OBJECTS];
    Object initial_objects[NO_OBJECTS];

    int rows = time_scale / log_step;
    int cols = NO_OBJECTS;

    Object *simulation_log = malloc(rows * cols * sizeof(Object));
    if (!simulation_log)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Earth - orbiting speed 30,000
    objects[0].mass = 5.972e24; // kg
    objects[0].motion.position = (Vec3){0.0f, 0.0f, 0.0};
    objects[0].motion.velocity = (Vec3){0.0f, 0.0f, 0.0f};
    objects[0].motion.force = (Vec3){0.0f, 0.0f, 0.0f};
    objects[0].symbol = 'E';

    // Moon
    objects[1].mass = 7.348e22;                                    // kg
    objects[1].motion.position = (Vec3){384400000.0f, 0.0f, 0.0f}; // meters from Earth
    objects[1].motion.velocity = (Vec3){0.0f, 1022.0f, 0.0f};      // m/s (orbital speed)
    objects[1].motion.force = (Vec3){0.0f, 0.0f, 0.0f};            // m/s (orbital speed)
    // moon orbital speed 1022.0f
    objects[1].symbol = 'M';

    // Satellite
    objects[2].mass = 6000;                                       // kg
    objects[2].motion.position = (Vec3){0.0f, 3.6e7f, 0.0f}; // meters from Earth
    objects[2].motion.velocity = (Vec3){3000.0f, 2000.0f, 2000.0f};  // m/s (orbital speed)
    objects[2].motion.force = (Vec3){0.0f, 0.0f, 0.0f};           // m/s (orbital speed)
    objects[2].symbol = 'S';

    /*
    // Sun
    objects[3].mass = 1.989e30;  // kg
    objects[3].motion.position = (Vec3){-150000000000.0f, 0.0f, 0.0f};  // meters from Earth
    objects[3].motion.velocity = (Vec3){0.0f, 0.0f, 0.0f};        // m/s (orbital speed)
    objects[3].motion.force = (Vec3){0.0f, 0.0f, 0.0f};        // m/s (orbital speed)
    objects[3].symbol = 'o';
    */

    memcpy(initial_objects, objects, sizeof(objects));

    // set initial values
    simulate(simulation_log, initial_objects, objects, time_scale);
    render_interactive(simulation_log, 0, false);
    program_ui(simulation_log, initial_objects, objects);
    
    /*
    // i timestep = delta_time
    for (int i = 0; i < (time_scale / delta_time) + 1; i++)
    {

        // log every log_step
        update_log(simulation_log, objects, i * delta_time);

        // render every day
        if (is_interval(DAY_INTERVAL, i))
        {
            display_time(i * delta_time);
            render_objects(objects, XY, 1);
            display_position(objects[0]);

        }


        apply_gravitational_forces_N(objects);
        update_N(objects);
    }

    */

    // render_objects(get_log_data(simulation_log, objects, WEEK - (DAY / 2)), XY, 1);
    free(simulation_log);

    return 0;
}

/*
    core physics
*/
// calculates the distance between two objects
double distance(Object object1, Object object2)
{
    double distanceX = object1.motion.position.x - object2.motion.position.x;
    double distanceY = object1.motion.position.y - object2.motion.position.y;
    double distanceZ = object1.motion.position.z - object2.motion.position.z;

    return sqrt(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ);
};

// applies the gravitational forces between two objects
void apply_gravitational_forces(Object *object1, Object *object2)
{
    Vec3 r;
    r.x = object2->motion.position.x - object1->motion.position.x;
    r.y = object2->motion.position.y - object1->motion.position.y;
    r.z = object2->motion.position.z - object1->motion.position.z;

    double distance = sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
    double force_magnitude = (GRAVITATIONAL_CONSTANT * object1->mass * object2->mass) / (distance * distance);

    Vec3 force;
    force.x = force_magnitude * r.x / distance;
    force.y = force_magnitude * r.y / distance;
    force.z = force_magnitude * r.z / distance;

    object1->motion.force.x += force.x;
    object1->motion.force.y += force.y;
    object1->motion.force.z += force.z;

    object2->motion.force.x -= force.x;
    object2->motion.force.y -= force.y;
    object2->motion.force.z -= force.z;
}

// applies the gravitational forces between all objects
void apply_gravitational_forces_N(Object objects[])
{

    for (int i = 0; i < NO_OBJECTS; i++)
    {
        objects[i].motion.force = (Vec3){0.0f, 0.0f, 0.0f};
    }

    for (int i = 0; i < (NO_OBJECTS - 1); i++)
    {
        for (int j = i + 1; j < NO_OBJECTS; j++)
        {
            apply_gravitational_forces(&objects[i], &objects[j]);
        }
    }
}

/*
    state updates
*/
// updates the velocity and position of a given object
void update(Object *object)
{
    Vec3 acceleration = {
        object->motion.force.x / object->mass,
        object->motion.force.y / object->mass,
        object->motion.force.z / object->mass};

    object->motion.velocity.x += acceleration.x * delta_time;
    object->motion.velocity.y += acceleration.y * delta_time;
    object->motion.velocity.z += acceleration.z * delta_time;

    object->motion.position.x += object->motion.velocity.x * delta_time;
    object->motion.position.y += object->motion.velocity.y * delta_time;
    object->motion.position.z += object->motion.velocity.z * delta_time;
}

// updates the velocity and position of all objects
void update_N(Object objects[])
{
    for (int i = 0; i < NO_OBJECTS; i++)
    {
        update(&objects[i]);
    }
}

/*
    simulation log
*/
// writes all the objects motion data to the simulation log every log step interval
void update_log(Object *sim_log, Object objects[], int time_seconds)
{
    if (is_interval(log_step, time_seconds))
    {
        int index = (time_seconds / log_step);
        for (int i = 0; i < NO_OBJECTS; i++)
        {
            sim_log[index * NO_OBJECTS + i].motion = objects[i].motion;
            sim_log[index * NO_OBJECTS + i].mass = objects[i].mass;
            sim_log[index * NO_OBJECTS + i].symbol = objects[i].symbol;
        }
    }
}

// retrieves log data
Object *get_log_data(Object *sim_log, int time_seconds)
{
    int index = (time_seconds / log_step);

    return &sim_log[index * NO_OBJECTS];
}

/*
    simulation control
*/
void simulate(Object *sim_log, Object initial_objects[], Object objects[], int time_seconds)
{
    memcpy(objects, initial_objects, NO_OBJECTS * sizeof(objects[0]));

    // i timestep = delta_time
    for (int i = 0; i < (time_seconds / delta_time) + 1; i++)
    {

        // log every log_step
        update_log(sim_log, objects, i * delta_time);

        apply_gravitational_forces_N(objects);
        update_N(objects);
    }
}

/*
    rendering
*/
// renders all the objects in ASCII in a given area
void render_objects_static(Object *sim_log, int time_seconds)
{
    //camera.angular_resolution_x = 2 * atan((1.07e9 / camera.no_pixelsX) / camera.view_size);
    camera.angular_resolution_x = 2 * atan(1.0 / camera.no_pixelsX);
    camera.angular_resolution_y = camera.angular_resolution_x * ((double)camera.no_pixelsX / camera.no_pixelsY);

    camera.pixel_size_x = camera.view_size / camera.no_pixelsX;
    camera.pixel_size_y = camera.view_size / camera.no_pixelsY;

    printf("pixelsizeX: %lf", camera.pixel_size_x);
    Vec3 focused_object_offset = (Vec3){0.0f, 0.0f, 0.0f};


    // number of pixels from the middle to the end
    int half_screen_sizeX = camera.no_pixelsX / 2;
    int half_screen_sizeY = camera.no_pixelsY / 2;

    char *plane_str;
    Vec3 display_pixel[NO_OBJECTS];
    int trail[NO_PIXELSX][NO_PIXELSY];
    char slope_position[NO_PIXELSX][NO_PIXELSY];

    int depths[NO_PIXELSX][NO_PIXELSY];
    double closest = 0.0;

    bool closest_initialised = false;


    printf("cameraX: %lf", cameraX);

    
    if (view_focused_object >= 0)
    {
        focused_object_offset.x = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.x;
        focused_object_offset.y = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.y;
        focused_object_offset.z = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.z;
    }

    
    for (int x = 0; x < NO_PIXELSX; x++)
    {
        for (int y = 0; y < NO_PIXELSY; y++)
        {
            trail[x][y] = 0;
            depths[x][y] = 0;
        }
    }

    bool displayed = false;
    Vec3 unrot_display_position; // perceived location when displaying, unrotated

    for (int i = 0; i < NO_OBJECTS; i++)
    {
        Vec3 object_position;
        Vec3 rot_display_position; // perceived location when dispalying, rotated
        double depth_ratio_x;
        double depth_ratio_y;
        double object_angle_size_x;
        double object_angle_size_y;

        object_position = get_log_data(sim_log, time_seconds)[i].motion.position;

        unrot_display_position.x = (object_position.x + focused_object_offset.x) - camera.pivot_position.x;
        unrot_display_position.y = (object_position.y + focused_object_offset.y) - camera.pivot_position.y;
        unrot_display_position.z = (object_position.z + focused_object_offset.z) - camera.pivot_position.z;

        rot_display_position = rotate_z_up(unrot_display_position, degrees.z, degrees.x);

        double object_depth = -rot_display_position.z + ((camera.view_size / 2) / zoom); // distance in metres from the camera to the object

        object_angle_size_x = 2 * atan((camera.pixel_size_x) / (object_depth * 2));
        object_angle_size_y = 2 * atan((camera.pixel_size_y) / (object_depth * 2));

        depth_ratio_x = camera.angular_resolution_x / object_angle_size_x;
        depth_ratio_y = camera.angular_resolution_y / object_angle_size_y;

        if (depth_ratio_x > 0 && depth_ratio_y > 0)
        {
            display_pixel[i].x = (int)((rot_display_position.x / (camera.pixel_size_x * depth_ratio_x)) + (half_screen_sizeX));
            display_pixel[i].y = (int)((camera.no_pixelsY) - ((rot_display_position.y / (camera.pixel_size_y * depth_ratio_y )) + (half_screen_sizeY)));
        }

    }

    int trailx;
    int traily;

    float ratio;

    // idea: introduce different colours for depth?

    
    for (int i = 0; i < (time_scale / log_step); i++)
    {
        
        Vec3 orbit_offset = (Vec3){0.0f,0.0f,0.0f};
        Vec3 rot_display_position; // perceived location when dispalying, rotated
        double object_depth;

        if (motion_relative_to_object >= 0)
        {
            // movement relative to the object
            orbit_offset.x = (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.x) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.x;

            orbit_offset.y = (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.y) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.y;

            orbit_offset.z = (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.z) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.z;
        }

        for (int j = 0; j < NO_OBJECTS; j++)
        {
            
            Vec3 object_position;
            Vec3 unrot_display_position;
            double depth_ratio_x;
            double depth_ratio_y;
            double object_angle_size_x;
            double object_angle_size_y;

            object_position = get_log_data(sim_log, i * log_step)[j].motion.position;

            unrot_display_position.x = (object_position.x + focused_object_offset.x + orbit_offset.x) - camera.pivot_position.x;
            unrot_display_position.y = (object_position.y + focused_object_offset.y + orbit_offset.y) - camera.pivot_position.y;
            unrot_display_position.z = (object_position.z + focused_object_offset.z + orbit_offset.z) - camera.pivot_position.z;

            rot_display_position = rotate_z_up(unrot_display_position, degrees.z, degrees.x);

            object_depth = -rot_display_position.z + ((camera.view_size / 2) / zoom); // distance in metres from the camera to the object   

            object_angle_size_x = 2 * atan((camera.pixel_size_x) / (object_depth * 2));
            object_angle_size_y = 2 * atan((camera.pixel_size_y) / (object_depth * 2));
            
            depth_ratio_x = camera.angular_resolution_x / object_angle_size_x;
            depth_ratio_y = camera.angular_resolution_y / object_angle_size_y;

            trailx = (int)((rot_display_position.x / (camera.pixel_size_x * depth_ratio_x)) + (half_screen_sizeX));
            traily = (int)((camera.no_pixelsY) - ((rot_display_position.y / (camera.pixel_size_y * depth_ratio_y)) + (half_screen_sizeY)));



            if (trailx >= 0 && trailx < NO_PIXELSX && traily >= 0 && traily < NO_PIXELSY && (depth_ratio_x > 0 && depth_ratio_y > 0))
            {
                Vec3 velocity;
                Vec3 vrot;

                velocity.x = get_log_data(sim_log, i * log_step)[j].motion.velocity.x;
                velocity.y = get_log_data(sim_log, i * log_step)[j].motion.velocity.y;
                velocity.z = get_log_data(sim_log, i * log_step)[j].motion.velocity.z;

                vrot = rotate_z_up(velocity, degrees.z, degrees.x);

                if (!closest_initialised)
                {
                    closest = object_depth;
                    closest_initialised = true;
                    printf("trailx: %d", trailx);
                    printf("traily: %d\n", traily);
                }
                else if (object_depth < closest)
                {
                    closest = object_depth;
                }

                
                if (trail[trailx][traily] == 1)
                {
                    if (object_depth < depths[trailx][traily])
                    {
                        depths[trailx][traily] = object_depth;
                    }
                }
                else
                {
                    trail[trailx][traily] = 1;
                    depths[trailx][traily] = object_depth;
                } 



                if (fabs(vrot.x) < 1e-6)
                    vrot.x = 1e-6; // avoid division by zero
                ratio = vrot.y / vrot.x;

                if (ratio > 4.0)
                {
                    slope_position[trailx][traily] = '|'; // steep upward
                }
                else if (ratio > 0.5)
                {
                    slope_position[trailx][traily] = '/'; // moderate upward
                }
                else if (ratio > -0.5)
                {
                    slope_position[trailx][traily] = '='; // mostly horizontal
                }
                else if (ratio > -4.0)
                {
                    slope_position[trailx][traily] = '\\'; // moderate downward
                }
                else
                {
                    slope_position[trailx][traily] = '|'; // steep downward
                }

            }
        }
    }
    
    char frame[FRAME_BUFFER_SIZE];
    int idx = 0;

    // Clear & home ANSI codes
    idx += sprintf(&frame[idx], "\033[2J\033[H");

    // Header text
    idx += sprintf(&frame[idx], "\n\n%s", display_time(time_seconds));
    idx += sprintf(&frame[idx], "\n|   ZOOM: \033[36m%4.3fx\033[0m   ", zoom);
    idx += sprintf(&frame[idx], "|   RESOLUTION: \033[36m%s\033[0m   ", format_number(camera.pixel_size_x / zoom));
    idx += sprintf(&frame[idx], "|   WIDTH: \033[36m%s\033[0m   |", format_number((camera.view_size) / zoom));
    idx += sprintf(&frame[idx], "   YAW: \033[36m%3d\033[0m | PITCH: \033[36m%3d\033[0m   |\n", (int)degrees.z % 360, (int)degrees.x % 360);


    for (int y = 0; y < camera.no_pixelsY; y++)
    {
        for (int x = 0; x < camera.no_pixelsX; x++)
        {
            bool drawn = false;
            // Draw objects
            for (int ob = 0; ob < NO_OBJECTS; ob++)
            {
                if (display_pixel[ob].x == x && display_pixel[ob].y == y)
                {
                    idx += sprintf(
                        &frame[idx],
                        " \033[32m%c\033[0m ",
                        get_log_data(sim_log, time_seconds)[ob].symbol
                    );
                    drawn = true;
                    break;
                }
            }

            // Draw trail with depth coloring
            if (!drawn && trail[x][y] == 1)
            {
                char c = slope_position[x][y];
                double d = depths[x][y];
                
                // Avoid divide-by-zero
                double fraction = (closest > 1e-9) ? ((d - closest) / closest) : 0.0;

                // Clamp to non-negative
                if (fraction < 0) fraction = 0;

                            // Depth → colour based on fractional distance
                if (fraction > 1.0)      // >100% farther
                    idx += sprintf(&frame[idx], "\033[34m %c \033[0m", c); // blue (very far)
                else if (fraction > 0.50) // +50% farther
                    idx += sprintf(&frame[idx], "\033[36m %c \033[0m", c); // cyan
                else if (fraction > 0.25) // +25% farther
                    idx += sprintf(&frame[idx], "\033[32m %c \033[0m", c); // green
                else if (fraction > 0.10) // +10% farther
                    idx += sprintf(&frame[idx], "\033[33m %c \033[0m", c); // yellow/orange
                else                     // within +10% of the closest
                    idx += sprintf(&frame[idx], "\033[31m %c \033[0m", c); // red (very near)

                drawn = true;
            }

            // Empty pixel
            if (!drawn)
            {
                idx += sprintf(&frame[idx], " . ");
            }
        }

        idx += sprintf(&frame[idx], "\n");
    }

    // Print the entire frame at once
    printf("%s", frame);

}

// interactive version of the advanced renderer at a snapshot
char render_interactive(Object *sim_log, int time_seconds, bool have_time_control)
{

    double extra_move = 1;
    char input_str[32];



    while (1)
    {
        render_objects_static(sim_log, time_seconds);

       

        if (have_time_control)
            printf("[ TIME: ENTER > | b < ]   ");

        printf("[ ZOOM: - | z0 | + ]   [ QUIT ]");

        if (fgets(input_str, sizeof(input_str), stdin) == NULL)
            return '1';

        // Remove newline if present
        input_str[strcspn(input_str, "\n")] = 0;
        if(strlen(input_str) == 0)
        {
            if(have_time_control)
                return '>';
        }
        else if (strcmp(input_str, "b") == 0)
        {
            if(have_time_control)
                return '<';
        }
        else if (strcmp(input_str, "+") == 0)
        {
            zoom *= 2;
        }
        else if (strcmp(input_str, "-") == 0)
        {
            zoom /= 2;
        }
        else if (input_str[0] == 'z')
        {
            zoom = pow(2, atof(input_str + 1));
        }
        else if (strcmp(input_str, "i") == 0)
        {
            display_all_information(get_log_data(sim_log, time_seconds));
            getchar();
        }
        else if(input_str[0] == 'e')
        {

            
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){0,0,-1}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);
            
        }
        else if(input_str[0] == 'q')
        {

            
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){0,0,1}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);
            
        }
        else if(input_str[0] == 'w')
        {

            
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){0,1,0}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);
            
        }
        else if(input_str[0] == 's')
        {

            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){0,-1,0}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);
        
        }
        else if(input_str[0] == 'd')
        {
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){1,0,0}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);
            
        }
        else if(input_str[0] == 'a')
        {

            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            pan_camera((Vec3){-1,0,0}, extra_move * calculate_resolution(), -degrees.x, -degrees.z);

        }
        else if(strncmp(input_str, "yaw", 3) == 0)
        {
            int result = atoi(input_str + 3);
            if (result == 0)
                degrees.z = 0;
            else
                degrees.z += atoi(input_str + 3);

        }
        else if(strncmp(input_str, "pitch", 5) == 0)
        {
            int result = atoi(input_str + 5);
            if (result == 0)
                degrees.x = 0;
            else
                degrees.x += atoi(input_str + 5);

        }
        
        else if(strcmp(input_str, "rotate") == 0)
        {
            rotate_render(sim_log, time_seconds);
        }
        else if (strcmp(input_str, "-1") == 0)
        {
            return '0';
        }
        else
        {
            // Unrecognized input
        }

        extra_move = 1;
    }
}

// interactive version of the advanced renderer over time
void render_objects_playback(Object *sim_log, int start, int end)
{
    int i = (start / render_step);
    char return_code;

    while (i < (end / render_step) + 1)
    {
        return_code = render_interactive(sim_log, i * render_step, true);

        switch (return_code)
        {
        case '0':
            return;

        case '>':
            i++;
            break;
        
        case '<':
            i--;
            break;

        default:
            break;
        }

    }

}

void rotate_render(Object *sim_log, int time_seconds)
{
    for (int i = 0; i < 360; i+= 5)
    {
        render_objects_static(sim_log, time_seconds);
        degrees.z += 5;
        Sleep(10);
    }
}

Vec3 rotate_z_up(Vec3 v, double spin_deg, double pitch_deg)
{
    double spin  = spin_deg * (M_PI / 180.0);
    double pitch = pitch_deg * (M_PI / 180.0);

    double cs = cos(spin);
    double ss = sin(spin);
    double cp = cos(pitch);
    double sp = sin(pitch);

     // --- Rotate around world Z (spin/yaw) ---
    double x1 = cs * v.x - ss * v.y;
    double y1 = ss * v.x + cs * v.y;
    double z1 = v.z;

    // --- Rotate around local X (pitch) ---
    double y2 = cp * y1 - sp * z1;
    double z2 = sp * y1 + cp * z1;
    double x2 = x1;

    Vec3 out = {x2, y2, z2};
    return out;
}

void pan_camera(Vec3 direction, double move, double pitch, double yaw)
{
    // Create the pitch and yaw matrices
    Mat3 pitch_matrix = create_pitch_matrix(pitch * (M_PI / 180.0));
    Mat3 yaw_matrix = create_yaw_matrix(yaw * (M_PI / 180.0));

    // Apply pitch rotation to the direction vector
    Vec3 rotated_direction = mat3_multiply_vec3(pitch_matrix, direction);

    // Apply yaw rotation to the rotated direction vector
    rotated_direction = mat3_multiply_vec3(yaw_matrix, rotated_direction);

    // Apply movement based on the new rotated direction
    // Assuming camera's pivot_position is a Vec3
    camera.pivot_position.x += rotated_direction.x * move;
    camera.pivot_position.y += rotated_direction.y * move;
    camera.pivot_position.z += rotated_direction.z * move;
    
}

Vec3 mat3_multiply_vec3(Mat3 mat, Vec3 vec) {
    Vec3 result;
    result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z;
    result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z;
    result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z;
    return result;
}

// Create a pitch rotation matrix (rotation around X-axis)
Mat3 create_pitch_matrix(double pitch) {
    Mat3 mat = {
        .m = {
            {1, 0, 0},
            {0, cos(pitch), -sin(pitch)},
            {0, sin(pitch), cos(pitch)}
        }
    };
    return mat;
}

// Create a yaw rotation matrix (rotation around Z-axis)
Mat3 create_yaw_matrix(double yaw) {
    Mat3 mat = {
        .m = {
            {cos(yaw), -sin(yaw), 0},
            {sin(yaw), cos(yaw),  0},
            {0, 0, 1}
        }
    };
    return mat;
}

/*
    utility
*/
// converts the current time in seconds to a human readable time format
char *display_time(int time_seconds)
{
    static char time_str[60];
    int days = 0;
    int hours = 0;
    int minutes = 0;

    if (DAY_INTERVAL > 0)
    {
        days = time_seconds / DAY;
    }

    if (HOUR_INTERVAL > 0)
    {
        if (DAY_INTERVAL > 0)
            hours = (time_seconds % DAY) / HOUR;
        else
            hours = time_seconds / HOUR;
    }

    if (HOUR_INTERVAL > 0)
    {
        minutes = time_seconds % HOUR;
    }
    else
    {
        minutes = time_seconds; // if HOUR_INTERVAL is 0, just show total steps as minutes
    }

    // printf("\n| DAY: %d | HOUR: %d | MINUTE: %d |\n", days, hours, minutes);
    snprintf(time_str, sizeof(time_str), "| TIME: DAY \033[36m%2d\033[0m | HOUR \033[36m%2d\033[0m | MINUTE \033[36m%2d\033[0m", days, hours, minutes);
    strcat(time_str, "\033[0m |");
    return time_str;
}

char *format_number(double number)
{
    static char number_str[60];
    if (fabs(number) >= 1e15)
        snprintf(number_str, sizeof(number_str), "%.3e", number);
    else if (fabs(number) >= 1e12)
        snprintf(number_str, sizeof(number_str), "%.2f T", number / 1e12);
    else if (fabs(number) >= 1e9)
        snprintf(number_str, sizeof(number_str), "%.2f B", number / 1e9);
    else if (fabs(number) >= 1e6)
        snprintf(number_str, sizeof(number_str), "%.2f M", number / 1e6);
    else
        snprintf(number_str, sizeof(number_str), "%.0f", number);

    return number_str;
}

double calculate_resolution()
{
    int half_screen_sizeX = NO_PIXELSX / 2;

    return ((double)RENDER_SIZE / half_screen_sizeX) / zoom;

}


// displays the position of a given object
void display_position(Object object)
{
    printf("\nx: %e", object.motion.position.x);
    printf("\ny: %e", object.motion.position.y);
    printf("\nz: %e\n", object.motion.position.z);
};

void display_all_information(Object objects[])
{
    for (int i = 0; i < NO_OBJECTS; i++)
    {
        printf("\n");
        for (int i = 0; i < 50; i++)
        {
            printf("-");
        }

        printf("\nObject: %c\n", objects[i].symbol);
        for (int i = 0; i < 50; i++)
        {
            printf("-");
        }

        printf("\nMass:");
        printf("\nkg: %s", format_number(objects[i].mass));

        printf("\n\nPosition:");
        printf("\nx: %s m", format_number(objects[i].motion.position.x));
        printf("\ny: %s m", format_number(objects[i].motion.position.y));
        printf("\nz: %s m", format_number(objects[i].motion.position.z));

        printf("\n\nVelocity:");
        printf("\nx: %s m/s", format_number(objects[i].motion.velocity.x));
        printf("\ny: %s m/s", format_number(objects[i].motion.velocity.y));
        printf("\nz: %s m/s", format_number(objects[i].motion.velocity.z));

        printf("\n\nForce:");
        printf("\nx: %s N", format_number(objects[i].motion.force.x));
        printf("\ny: %s N", format_number(objects[i].motion.force.y));
        printf("\nz: %s N\n\n", format_number(objects[i].motion.force.z));
    }
};

// returns true if step is at a given time interval
bool is_interval(int interval, int step)
{
    if (interval == 0)
    {
        return false;
    }

    return step % interval == 0;
}

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/*
    ui
*/
int program_ui(Object *sim_log, Object initial_objects[], Object objects[])
{
    intro();
    int user_choice = 0;
    do
    {
        printf("\nHere are your options:\n");
        printf("  - Start simulation (1)\n");
        printf("  - Adjust settings (2)\n");
        printf("  - Exit the program (-1)\n");
        scanf("%d", &user_choice);

        switch (user_choice)
        {
        case 1:
            simulation_ui(sim_log, initial_objects, objects);
            break;

        case 2:
            settings_ui();
        default:
            break;
        }

    } while (user_choice != -1);

    return 0;
}

int simulation_ui(Object *sim_log, Object initial_objects[], Object objects[])
{
    int user_choice;
    int time_seconds, days, hours, minutes;
    int time_seconds_start, time_seconds_end;

    menu_banner(1);

    do
    {
        printf("\nHere are your options:\n");
        printf("  - Display initial simulation state (1)\n");
        printf("  - Run simulation for a period (2)\n");
        printf("  - Render simulation for a period (3)\n");
        printf("  - Return to main menu (-1)\n");

        scanf("%d", &user_choice);
        switch (user_choice)
        {

        case 1:
            render_interactive(sim_log, 0, false);
            //render_objects_static(sim_log, 0, 0, time_scale);
            break;

        case 2:
            printf("\nThe current delta time is: %d seconds", delta_time);
            printf("\nThe current log step is: %d seconds", log_step);
            printf("\nHow long do you want to run the simulation for? Enter in the format: days hours minutes (e.g., 7 0 0):\n");

            scanf("%d %d %d", &days, &hours, &minutes);

            time_seconds = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);
            time_scale = time_seconds;
            simulate(sim_log, initial_objects, objects, time_seconds);
            printf("\nSimulation successfully ran for %s\n", display_time(time_seconds));
            break;

        case 3:
            printf("\nThe current delta time is: %d seconds", delta_time);
            printf("\nThe current log step is: %d seconds", log_step);
            printf("\nThe current render step is: %s", display_time(render_step));
            printf("\nThe simulation has ran for: %s", display_time(time_scale));
            printf("\nBetween what two times do you want to render the simulation for? Enter in the format: days hours minutes (e.g., 7 0 0):\n");

            printf("Start: ");
            scanf("%d %d %d", &days, &hours, &minutes);
            time_seconds_start = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);

            printf("\nEnd: ");
            scanf("%d %d %d", &days, &hours, &minutes);
            time_seconds_end = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);
            clear_input_buffer();

            //render_objects_playback(sim_log, time_seconds_start, time_seconds_end);
            render_objects_playback(sim_log, time_seconds_start, time_seconds_end);
            break;

        default:
            break;
        }

    } while (user_choice != -1);

    menu_banner(0);
}

int settings_ui()
{
    int user_choice;
    menu_banner(2);

    do
    {
        printf("\nHere are your options:\n");
        printf("  - Adjust simulation settings (1)\n");
        printf("  - Adjust rendering settings (2)\n");
        printf("  - Return to main menu: (-1)\n");

        scanf("%d", &user_choice);

        switch (user_choice)
        {
        case 1:
            simulation_settings_ui();
            break;

        case 2:
            render_settings_ui();
            break;

        default:
            break;
        }

    } while (user_choice != -1);

    menu_banner(0);

    return 0;
}

int simulation_settings_ui()
{
    int user_choice;
    int time_seconds, days, hours, minutes;

    do
    {
        printf("\nHere are your options:\n");
        printf("  - Adjust delta time (1)\n");
        printf("  - Adjust log step (2)\n");
        printf("  - Return to previous menu (-1)\n");

        scanf("%d", &user_choice);

        switch (user_choice)
        {
        case 1:
            printf("\nDelta time refers to how often the simulation is updated. It can be thought of as the accuracy of the simulation\n");
            printf("The current delta time is %d seconds, meaning the simulation updates every %d seconds", delta_time, delta_time);
            printf("\nWhat do you want delta time to be? Enter in the format: days hours minutes (e.g., 7 0 0):\n");
            scanf("%d %d %d", &days, &hours, &minutes);

            time_seconds = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);

            if (time_seconds == 0)
            {
                delta_time = MINUTE;
            }
            else
            {
                if (time_seconds > log_step)
                {
                    log_step = time_seconds;
                }
                delta_time = time_seconds;
            }

            printf("\ndelta time reassigned successfully! Delta time is: %d seconds\n", delta_time);
            break;

        case 2:
            printf("\nLog step refers to how often an entry is written into the simulation record\n");
            printf("The current log step is %d seconds, meaning the simulation record is written into every %d seconds", log_step, log_step);
            printf("\nWhat do you want log step to be? Enter in the format: days hours minutes (e.g., 7 0 0):\n");
            scanf("%d %d %d", &days, &hours, &minutes);

            time_seconds = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);
            if (time_seconds < delta_time || time_seconds == 0)
            {
                log_step = delta_time;
            }
            else
            {
                log_step = time_seconds;
            }

            printf("\nlog step reassigned successfully! log step is: %d seconds\n", log_step);
            break;

        default:
            break;
        }

    } while (user_choice != -1);

    return 0;
}

int render_settings_ui()
{
    int user_choice;

    int time_seconds, days, hours, minutes;
    char *plane_str;

    do
    {
        printf("\nHere are your options:\n");
        printf("  - Adjust render step (1)\n");
        printf("  - Adjust zoom level (2)\n");
        printf("  - Change coordinate plane (3)\n");
        printf("  - Change walkthrough settings (4)\n");
        printf("  - Return to previous menu (-1)\n");

        scanf("%d", &user_choice);

        switch (user_choice)
        {
        case 1:
            printf("\nRender step refers to how often images are displayed\n");
            printf("The current render step is %s meaning that images are displayed at intervals of %s", display_time(render_step), display_time(render_step));
            printf("\nWhat do you want the render step to be? Enter in the format: days hours minutes (e.g., 7 0 0):\n");
            scanf("%d %d %d", &days, &hours, &minutes);
            time_seconds = (days * DAY) + (hours * HOUR) + (minutes * MINUTE);

            render_step = time_seconds;

            printf("\nRender step reassigned successfully! Render step is %s\n", display_time(render_step));
            break;

        case 2:
            printf("\nZoom level refers to how zoomed in the images are rendered\n");
            printf("The current zoom level is: %d", zoom);
            printf("\nWhat do you want the zoom level to be?\n");
            scanf("%d", &zoom);

            printf("\nZoom level reassigned successfully! Zoom step is %d\n", zoom);
            break;

        case 3:
            if (plane == XY)
            {
                plane_str = "XY";
            }
            else if (plane == YZ)
            {
                plane_str = "YZ";
            }
            else
            {
                plane_str = "XZ";
            }

            printf("\nCoordinate plane refers to what two axes make the rendered image\n");
            printf("The current coordinate plane is: %s", plane_str);
            printf("\nWhat do you want the coordinate plane to be? XY(0), YZ(1) or XZ(2)\n");
            scanf("%d", &plane);

            printf("\nCoordinate plane changed successfully!\n");
            break;

        case 4:
            printf("\nWalkthrough refers to if you want for each image to stay on the screen until dismissed\n");
            printf("\nThe current walkthrough setting is: %d", render_wait);
            printf("\nWhat do you want the walkthrough setting to be? True(1) or false(0)\n");
            scanf("%d", &render_wait);

            printf("\nWalkthrough setting changed successfully!\n");
            break;

        default:
            break;
        }

    } while (user_choice != -1);

    return 0;
}

void menu_banner(int menu)
{
    int border_length = 50;
    char *string;

    printf("\n\n\n");

    if (menu == 0)
    {
        string = "  | Main Menu |";
    }
    else if (menu == 1)
    {
        string = "| Simulation Menu |";
    }
    else if (menu == 2)
    {
        string = " | Settings Menu |";
    }

    for (int i = 0; i < border_length; i++)
    {
        printf("-");
    }

    printf("\n\t\t%s\n", string);

    for (int i = 0; i < border_length; i++)
    {
        printf("-");
    }
}

void intro()
{
    int border_length = 50;
    for (int i = 0; i < border_length; i++)
    {
        printf("-");
    }

    printf("\n\n\n\t      | Gravity Simulation |\n\n\n");

    for (int i = 0; i < border_length; i++)
    {
        printf("-");
    }

    printf("\nBy Aleks Zygarlowski\n\n\n");
}
