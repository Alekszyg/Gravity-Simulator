#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <windows.h>

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
#define NO_PIXELSX 33    // 17
#define NO_PIXELSY 41    // 21
int render_step = DAY;   // how often rendering occurs
bool render_wait = true; // pause after each render
float zoom = 1;          // render zoom level
double view_offsetX = 0;
double view_offsetY = 0;
double view_offsetZ = 0;
double cameraX = 0;
double cameraY = 0;



int view_focused_object = -1;      // what object is the view focused on
int motion_relative_to_object = -1; // displays motion relative to this object


int plane = XY; //

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
void rotate_point(double*, Vec3, Vec3);
Vec3 rotate_z_up(Vec3 v, double spin_deg, double pitch_deg);
Vec3 rotate_z_up_pivot(Vec3 v, Vec3 pivot, double spin_deg, double pitch_deg);
Vec3 pan_camera_relative(double dx, double dy, double move);
Vec3 rotate_point_2d(Vec3 p, double angle_degrees);

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
    Vec3 cameraresult;
    Vec3 static old_angle = (Vec3){0,0,0};
    Vec3 static rotation_offset = (Vec3){0,0,0};
    Vec3 cameracoords;

    // number of pixels from the middle to the end
    int half_screen_sizeX = NO_PIXELSX / 2;
    int half_screen_sizeY = NO_PIXELSY / 2;

    double pixel_sizeX = ((double)RENDER_SIZE / half_screen_sizeX) / zoom;
    double pixel_sizeY = ((double)RENDER_SIZE / half_screen_sizeY) / zoom;


    
    
    if (degrees.x != old_angle.x || degrees.y != old_angle.y || degrees.z != old_angle.z)
    {
        cameracoords = (Vec3){cameraX, cameraY, 0};
        cameraresult = rotate_point_2d(cameracoords, old_angle.z);
        
        old_angle = degrees;
        
        rotation_offset.x += (cameraresult.x * zoom * pixel_sizeX);
        rotation_offset.y += (cameraresult.y * zoom * pixel_sizeX);
        rotation_offset.z = 0; //1e10f;

        cameraX = 0;
        cameraY = 0;
    }
    
    printf("rotationx: %lf", rotation_offset.x);
    printf("rotationy: %lf", rotation_offset.y);
    printf("CameraX: %lf", cameraX);
    printf("CameraY: %lf", cameraY);
    
    
    
    


    Vec3 object_offset = (Vec3){rotation_offset.x, -rotation_offset.y, -rotation_offset.z};

    printf("object_offset.y: %lf", object_offset.y);
    printf("view_offsetY: %lf", view_offsetY);

    char *plane_str;
    Vec3 coordinates[NO_OBJECTS];
    int trail[NO_PIXELSX][NO_PIXELSY];
    char slope_position[NO_PIXELSX][NO_PIXELSY];

    int depths[NO_PIXELSX][NO_PIXELSY];
    double closest = 0.0;

    bool closest_initialised = false;

    //rotation_offset.x = (cameraX * zoom * pixel_sizeX);
    //rotation_offset.y = (cameraY * zoom * pixel_sizeX);

    printf("cameraX: %lf", cameraX);

    if (view_focused_object >= 0)
    {
        object_offset.x = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.x + (rotation_offset.x);
        object_offset.y = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.y - (rotation_offset.y);
        object_offset.z = -1 * get_log_data(sim_log, time_seconds)[view_focused_object].motion.position.z - (rotation_offset.z);
    }

    for (int x = 0; x < NO_PIXELSX; x++)
    {
        for (int y = 0; y < NO_PIXELSY; y++)
        {
            trail[x][y] = 0;
        }
    }

    bool displayed = false;
    double rotated[3];

    for (int i = 0; i < NO_OBJECTS; i++)
    {
        if (plane == XY)
        {
            
            Vec3 point;
            Vec3 result;

            point.x = get_log_data(sim_log, time_seconds)[i].motion.position.x + object_offset.x;
            point.y = get_log_data(sim_log, time_seconds)[i].motion.position.y + object_offset.y;
            point.z = get_log_data(sim_log, time_seconds)[i].motion.position.z + object_offset.z;


            //rotate_point(rotated, point, degrees);
            result = rotate_z_up(point, degrees.z, degrees.x);
            //result = rotate_z_up_pivot(point, cameracoords, degrees.z, degrees.x);

            coordinates[i].x = ((int)((result.x) / pixel_sizeX) + (half_screen_sizeX)) + (cameraX * zoom); // + (resultcoords.x);
            coordinates[i].y = ((int)((result.y) / pixel_sizeY) + (half_screen_sizeY)) - (cameraY * zoom); // - (resultcoords.y);
            plane_str = "| VIEW: X <> | Y v^ |";

            //coordinates[i].x = (int)((get_log_data(sim_log, time_seconds)[i].motion.position.x + object_offset.x) / pixel_sizeX) + (half_screen_sizeX);
            //coordinates[i].y = (int)((get_log_data(sim_log, time_seconds)[i].motion.position.y + object_offset.y) / pixel_sizeY) + (half_screen_sizeY);
        }
        else if (plane == YZ)
        {
            plane_str = "| VIEW: Y <> | Z v^ |";
            coordinates[i].x = ((get_log_data(sim_log, time_seconds)[i].motion.position.y + object_offset.y) / pixel_sizeX) + (half_screen_sizeX);
            coordinates[i].y = ((get_log_data(sim_log, time_seconds)[i].motion.position.z + object_offset.z) / pixel_sizeY) + (half_screen_sizeY);
        }
        else if (plane == XZ)
        {
            plane_str = "| VIEW: X <> | Z v^ |";
            coordinates[i].x = ((get_log_data(sim_log, time_seconds)[i].motion.position.x + object_offset.x) / pixel_sizeX) + (half_screen_sizeX);
            coordinates[i].y = ((get_log_data(sim_log, time_seconds)[i].motion.position.z + object_offset.z) / pixel_sizeY) + (half_screen_sizeY);
        }
    }

    printf("\n\n%s", display_time(time_seconds));
    printf("   ZOOM: \033[36m%4.3fx\033[0m   ", zoom);
    printf("|   RESOLUTION: \033[36m%s\033[0m   ", format_number(pixel_sizeX));
    printf("|   WIDTH: \033[36m%s\033[0m   |", format_number((RENDER_SIZE * 2) / zoom));
    printf("\n%s\n", plane_str);
    int trailx;
    int traily;

    double orbit_offsetX = object_offset.x;
    double orbit_offsetY = object_offset.y;
    double orbit_offsetZ = object_offset.z;

    float ratio;

    // idea: introduce different colours for depth?

    for (int i = 0; i < (time_scale / log_step); i++)
    {
        

        Vec3 point;
        Vec3 result;
        double rotated[3];
        if (motion_relative_to_object >= 0)
        {
            

            // movement relative to the object
            orbit_offsetX = object_offset.x + (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.x) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.x;

            orbit_offsetY = object_offset.y + (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.y) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.y;

            orbit_offsetZ = object_offset.z + (-1 * get_log_data(sim_log, i * log_step)[motion_relative_to_object].motion.position.z) +
                            get_log_data(sim_log, time_seconds)[motion_relative_to_object].motion.position.z;
        }

        for (int j = 0; j < NO_OBJECTS; j++)
        {
            double depth;

            if (plane == XY)
            {
                
                point.x = get_log_data(sim_log, i * log_step)[j].motion.position.x + orbit_offsetX;
                point.y = get_log_data(sim_log, i * log_step)[j].motion.position.y + orbit_offsetY;
                point.z = get_log_data(sim_log, i * log_step)[j].motion.position.z + orbit_offsetZ;

                rotate_point(rotated, point, degrees);
                result = rotate_z_up(point, degrees.z, degrees.x);

                trailx = (int)((result.x) / pixel_sizeX) + (half_screen_sizeX) + (cameraX * zoom);
                traily = (int)((NO_PIXELSY - 1) - ((int)(((result.y) / pixel_sizeY) + (half_screen_sizeY)))) + (cameraY * zoom);
            }
            else if (plane == YZ)
            {
                trailx = ((get_log_data(sim_log, i * log_step)[j].motion.position.y + orbit_offsetY) / pixel_sizeX) + (half_screen_sizeX);
                traily = (NO_PIXELSY - 1) - (int)(((get_log_data(sim_log, i * log_step)[j].motion.position.z + orbit_offsetZ) / pixel_sizeY) + (half_screen_sizeY));
            }
            else if (plane == XZ)
            {
                trailx = ((get_log_data(sim_log, i * log_step)[j].motion.position.x + orbit_offsetX) / pixel_sizeX) + (half_screen_sizeX);
                traily = (NO_PIXELSY - 1) - (int)(((get_log_data(sim_log, i * log_step)[j].motion.position.z + orbit_offsetZ) / pixel_sizeY) + (half_screen_sizeY));
            }

            if (trailx >= 0 && trailx < NO_PIXELSX && traily >= 0 && traily < NO_PIXELSY)
            {

                Vec3 velocity;
                Vec3 vrot;
                double vrotation[3];

                velocity.x = get_log_data(sim_log, i * log_step)[j].motion.velocity.x;
                velocity.y = get_log_data(sim_log, i * log_step)[j].motion.velocity.y;
                velocity.z = get_log_data(sim_log, i * log_step)[j].motion.velocity.z;

                vrot = rotate_z_up(velocity, degrees.z, degrees.x);

                depth = -result.z;

                /*
                // depth and slope for planes
                if (plane == XY)
                {
                    //depth = -1 * (get_log_data(sim_log, i * log_step)[j].motion.position.z);
                    depth = -1 * rotated[2];
                    vx = get_log_data(sim_log, i * log_step)[j].motion.velocity.x;
                    vy = get_log_data(sim_log, i * log_step)[j].motion.velocity.y;

                }
                else if (plane == YZ)
                {
                    depth = -1 * (get_log_data(sim_log, i * log_step)[j].motion.position.x);

                    vx = get_log_data(sim_log, i * log_step)[j].motion.velocity.y;
                    vy = get_log_data(sim_log, i * log_step)[j].motion.velocity.z;
                }
                else if (plane == XZ)
                {
                    depth = 1 * (get_log_data(sim_log, i * log_step)[j].motion.position.y);

                    vx = get_log_data(sim_log, i * log_step)[j].motion.velocity.x;
                    vy = get_log_data(sim_log, i * log_step)[j].motion.velocity.z;

                }

                */

                if (!closest_initialised)
                {
                    closest = depth;
                }
                else if (depth < closest)
                {
                    closest = depth;
                    closest_initialised = true;
                }

                depths[trailx][traily] = depth;
                trail[trailx][traily] = 1;




                if (fabs(vrot.x) < 1e-6)
                    vrot.x = 1e-6; // avoid division by zero
                ratio = vrot.y / (vrot.x + 1.0);

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




    for (int y = 0; y < NO_PIXELSY; y++)
    {
        for (int x = 0; x < NO_PIXELSX; x++)
        {

            for (int ob = 0; ob < NO_OBJECTS; ob++)
            {
                if ((int)coordinates[ob].x == x && ((NO_PIXELSY - 1) - (int)coordinates[ob].y) == y)
                {
                    printf(" \033[32m%c\033[0m ", get_log_data(sim_log, 0)[ob].symbol);
                    displayed = true;
                    break;
                }
            }

            if (!displayed && trail[x][y] == 1)
            {

                if (depths[x][y] >= closest + (pixel_sizeX * 4))
                {
                    printf("\033[34m %c \033[0m", slope_position[x][y]); // blue
                }
                else if (depths[x][y] >= closest + (pixel_sizeX * 3))
                {
                    printf("\033[36m %c \033[0m", slope_position[x][y]); // cyan
                }
                else if (depths[x][y] >= closest + (pixel_sizeX * 2))
                {
                    printf("\033[32m %c \033[0m", slope_position[x][y]); // green
                }
                else if (depths[x][y] >= closest + pixel_sizeX)
                {
                    printf("\033[33m %c \033[0m", slope_position[x][y]); // orange/yellow
                }
                else
                {
                    printf("\033[31m %c \033[0m", slope_position[x][y]); // red (closest)
                }

                displayed = true;

            }

            if (!displayed)
            {
                printf(" . ");
            }

            displayed = false;
        }

        printf("\n");
    }
}

// interactive version of the advanced renderer at a snapshot
char render_interactive(Object *sim_log, int time_seconds, bool have_time_control)
{
    // plane hud's
    char *hud_XY = "[ VIEW: 0 XY ( * ) | 1 YZ ( 90*> r90*< ) | 2 XZ ( 90*v ) ]";
    char *hud_YZ = "[ VIEW: 0 XY ( 90*^ r90*< ) | 1 YZ ( * ) | 2 XZ ( 90*< ) ]";
    char *hud_XZ = "[ VIEW: 0 XY ( 90*^ ) | 1 YZ ( 90*> ) | 2 XZ ( * ) ]";
    char *plane_hud;

    double extra_move = 1;
    char input_str[32];



    while (1)
    {
        render_objects_static(sim_log, time_seconds);

        // change hud based on plane
        switch (plane)
        {
        case XY:
            plane_hud = hud_XY;
            break;
        case YZ:
            plane_hud = hud_YZ;
            break;
        case XZ:
            plane_hud = hud_XZ;
            break;
        }

        if (have_time_control)
            printf("[ TIME: ENTER > | b < ]   ");

        printf("[ ZOOM: - | z0 | + ]   %s   [ QUIT ]", plane_hud);
        printf("\nSPIN: %lf PITCH %lf", degrees.z, degrees.x);

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
        else if (input_str[0] == 'p')
        {
            //zoom = pow(2, atoi(input_str + 1));
        }
        else if (strcmp(input_str, "0") == 0 || strcmp(input_str, "1") == 0 || strcmp(input_str, "2") == 0)
        {
            plane = input_str[0] - '0';
        }
        else if (strcmp(input_str, "i") == 0)
        {
            display_all_information(get_log_data(sim_log, time_seconds));
            getchar();
        }
        else if(input_str[0] == 'w')
        {

            
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            cameraY += extra_move / zoom;
            /*
            Vec3 m = pan_camera_relative(0, 1, extra_move * calculate_resolution());
            view_offsetX -= m.x;
            view_offsetY -= m.y;
            view_offsetZ -= m.z;
            */

            /*
            if (plane == XY)
                view_offsetY -= extra_move * calculate_resolution();
            if (plane == YZ)
                view_offsetZ -= extra_move * calculate_resolution();
            if (plane == XZ)
                view_offsetZ -= extra_move * calculate_resolution();
            */
        }
        else if(input_str[0] == 's')
        {

            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }


            cameraY -= extra_move / zoom;

            /*
            Vec3 m = pan_camera_relative(0, -1, extra_move * calculate_resolution());
            view_offsetX -= m.x;
            view_offsetY -= m.y;
            view_offsetZ -= m.z;
            */




            /*
            if (plane == XY)
                view_offsetY += extra_move * calculate_resolution();
            if (plane == YZ)
                view_offsetZ += extra_move * calculate_resolution();
            if (plane == XZ)
                view_offsetZ += extra_move * calculate_resolution();
            */
        }
        else if(input_str[0] == 'd')
        {
            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            cameraX -= extra_move / zoom;
            /*
            Vec3 m = pan_camera_relative(1, 0, extra_move * calculate_resolution());
            view_offsetX -= m.x;
            view_offsetY -= m.y;
            view_offsetZ -= m.z;
            */



            /*
            if (plane == XY)
                view_offsetX -= extra_move * calculate_resolution();
            if (plane == YZ)
                view_offsetY -= extra_move * calculate_resolution();
            if (plane == XZ)
                view_offsetX -= extra_move * calculate_resolution();
            */
        }
        else if(input_str[0] == 'a')
        {

            if(strlen(input_str) > 1)
            {
                extra_move = atoi(input_str + 1);
            }

            cameraX += extra_move / zoom;




            /*
             Vec3 m = pan_camera_relative(-1, 0, extra_move * calculate_resolution());
            view_offsetX -= m.x;
            view_offsetY -= m.y;
            view_offsetZ -= m.z;
            */

            /*
            if (plane == XY)
                view_offsetX += extra_move * calculate_resolution();
            if (plane == YZ)
                view_offsetY += extra_move * calculate_resolution();
            if (plane == XZ)
                view_offsetX += extra_move * calculate_resolution();
            */
        }
        else if(input_str[0] == 'x')
        {
            if(strlen(input_str) > 1)
            {
                degrees.x += atoi(input_str + 1);
            }

        }
        else if(input_str[0] == 'y')
        {
            if(strlen(input_str) > 1)
            {
                degrees.y += atoi(input_str + 1);
            }

        }
        else if(input_str[0] == 'z')
        {
            if(strlen(input_str) > 1)
            {
                degrees.z += atoi(input_str + 1);
            }

        }
        else if (strcmp(input_str, "q") == 0)
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

void rotate_point(double result[3], Vec3 point, Vec3 angles_degrees)
{
    // Convert to radians
    double ax = angles_degrees.x * M_PI / 180.0;
    double ay = angles_degrees.y * M_PI / 180.0;
    double az = angles_degrees.z * M_PI / 180.0;

    // Precompute cos/sin for each angle
    double cx = cos(ax), sx = sin(ax);
    double cy = cos(ay), sy = sin(ay);
    double cz = cos(az), sz = sin(az);

    // --- Rotate around X axis ---
    double y1 = point.y * cx - point.z * sx;
    double z1 = point.y * sx + point.z * cx;

    // --- Rotate around Y axis ---
    double x2 = point.x * cy + z1 * sy;
    double z2 = -point.x * sy + z1 * cy;

    // --- Rotate around Z axis ---
    double x3 = x2 * cz - y1 * sz;
    double y3 = x2 * sz + y1 * cz;

    // Return only the projected X, Y (for rendering)
    result[0] = x3;
    result[1] = y3;
    result[2] = z2;
}

Vec3 rotate_point_2d(Vec3 p, double angle_degrees)
{
    double a = angle_degrees * (M_PI / 180.0); // convert deg → rad
    double cs = cos(a);
    double sn = sin(a);

    Vec3 r;
    r.x = p.x * cs - p.y * sn;
    r.y = p.x * sn + p.y * cs;
    return r;
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

Vec3 rotate_z_up_pivot(Vec3 v, Vec3 pivot, double spin_deg, double pitch_deg)
{
    double spin  = spin_deg * (M_PI / 180.0);
    double pitch = pitch_deg * (M_PI / 180.0);

    double cs = cos(spin);
    double ss = sin(spin);
    double cp = cos(pitch);
    double sp = sin(pitch);

    // Translate so pivot is origin
    double x = v.x - pivot.x;
    double y = v.y - pivot.y;
    double z = v.z - pivot.z;

    // --- Rotate around world Z (spin/yaw) ---
    double x1 = cs * x - ss * y;
    double y1 = ss * x + cs * y;
    double z1 = z;

    // --- Rotate around local X (pitch) ---
    double y2 = cp * y1 - sp * z1;
    double z2 = sp * y1 + cp * z1;
    double x2 = x1;

    // Translate back to world space
    Vec3 out = { x2 + pivot.x, y2 + pivot.y, z2 + pivot.z };
    return out;
}


Vec3 pan_camera_relative(double dx, double dy, double move)
{
    Vec3 local;
    local.x = dx * move;
    local.y = dy * move;
    local.z = 0;

    // invert rotation (negative spin & pitch)
    Vec3 world = rotate_z_up(local, -degrees.z, -degrees.x);

    return world;
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
