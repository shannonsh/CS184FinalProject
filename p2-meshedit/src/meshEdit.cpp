#include "meshEdit.h"
#include "shaderUtils.h"
#include "GL/glew.h"

// Needed to generate stb_image binaries. Should only define in exactly one source file importing stb_image.h.
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

#define PI 3.14159265

#include <cmath>

Color ambient;
Color specular;
Color diffuse;

namespace CGL {


  void MeshEdit::init()
  {
    smoothShading = true;
    shadingMode = true;
    shaderProgID = loadShaders("../../shader/basic.vert", "../../shader/cel.frag");
    outlineShader = loadShaders("../../shader/outline.vert", "../../shader/outline.frag");
    load_textures();
    text_mgr.init(use_hdpi);
    text_color = Color(1.0, 1.0, 1.0);

    // Setup all the basic internal state to default values,
    // as well as some basic OpenGL state (like depth testing
    // and lighting).

    // Don't have anything selected initially.
    selectedFeature.invalidate();
    hoveredFeature.invalidate();

    // Set the integer bit vector representing which keys are down.
    left_down   = false;
    right_down  = false;
    middle_down = false;
    mouse_rotate = false;

    showHUD = false;
    camera_angles = Vector3D(0.0, 0.0, 0.0);

    mainGLSettings();

    // Initialize styles (colors, line widths, etc.) that will be used
    // to draw different types of mesh elements in various situations.
    initializeStyle();
  }
  
  void MeshEdit::mainGLSettings( void )
  {
    // 3D applications really like enabling the depth test,
    // this allows triangles that are closer to be drawn in
    // front of triangles that are farther away.
    /* Use depth buffering for hidden surface elimination. */
    glEnable(GL_DEPTH_TEST);
    
    // FIXME!
    // -- Setup some temporary working lights for now and resolve
    // -- the input light configurations later.
    
    // Red diffuse light.//.4
    GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat light_ambient[] = {.2, .2, .2, 1.0};
    // Infinite light location.
    // Note: Viewer is in the positive z direction,
    // so we make that component significant.
    GLfloat light_position[] = {.2, 0.5, 1.0, 0.0};
    /* Enable a single OpenGL light. */
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHT0);
    light_num++; // increment the number of lights currently turned on
    
    // Lighting needs to be explicitly enabled.
    glEnable(GL_LIGHTING);
    
    // Enable antialiasing and circular points.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_LINE_SMOOTH );
    //glEnable( GL_POLYGON_SMOOTH );
    glEnable(GL_POINT_SMOOTH);
    
    // cull backfaces for speed. Removes some UI stuff as a side effect tho
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    //glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
  }
  
  void MeshEdit::outlineGLSettings( void )
  {
    glDisable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
  }

  void MeshEdit::initializeStyle( void )
  {
    // Colors.
    defaultStyle.halfedgeColor = Color( 0.5, 0.5, 0.5 );
    hoverStyle.halfedgeColor = Color( 0.9, 0.9, 0.9 );
    selectStyle.halfedgeColor = Color( 1.0, 1.0, 1.0 );

    defaultStyle.faceColor = Color( 0.0, 0.50, 0.90 ); // HERE CUSTOM COLOR
    hoverStyle.faceColor = Color( 0.9, 0.75, 0.75 );
    selectStyle.faceColor = Color( 1.0, 1.00, 1.00 );

    defaultStyle.edgeColor = Color( 0.5, 0.5, 0.50 );
    hoverStyle.edgeColor = Color( 0.9, 0.0, 0.75 );
    selectStyle.edgeColor = Color( 1.0, 1.0, 1.00 );

    defaultStyle.vertexColor = Color( 0.0, 0.0, 0.00 );
    hoverStyle.vertexColor = Color( 0.8, 0.0, 0.75 );
    selectStyle.vertexColor = Color( 1.0, 1.0, 1.00 );

    // Primitive sizes.
    defaultStyle.strokeWidth = 1.0;
    hoverStyle.strokeWidth = 4.0;
    selectStyle.strokeWidth = 8.0;

    defaultStyle.vertexRadius = 2.0;
    hoverStyle.vertexRadius = 10.0;
    selectStyle.vertexRadius = 20.0;
  }

  void MeshEdit::render()
  {
    update_camera();
    draw_meshes();

    // // Draw the helpful picking messages.
    if (showHUD)
    {
      drawHUD();
    }

    return;
  }

  void MeshEdit::update_camera()
  {
    // Call resize() every time we draw, since it doesn't seem
    // to get called by the Viewer upon intial window creation
    // (this should probably be fixed!).
    GLint view[4];
    glGetIntegerv( GL_VIEWPORT, view );
    resize( view[2], view[3] );


    // Control the camera to look at the mesh.
    glMatrixMode(GL_MODELVIEW);

    // Clobber the old view matrices.
    glLoadIdentity();

    // Camera location.
    double cx, cy, cz;

    // up components.
    double up_x, up_y, up_z;
    up_x = up_y = up_z = 0.0;

    // Center of view coordinates.
    double v_x, v_y, v_z;
    v_x = view_focus.x;
    v_y = view_focus.y;
    v_z = view_focus.z;

    switch(up)
    {
      case Z_UP:

        up_z = 1.0;

        // x is horizontal screen rotational angle.
        // y is vertical screen rotational angle.
        cx = v_x + view_distance*sin(camera_angles.x)*cos(camera_angles.y*.99);
        cy = v_y + view_distance*cos(camera_angles.x)*cos(camera_angles.y*.99);
        cz = v_z + view_distance*sin(camera_angles.y);
        //cout << "Angles: " << camera_angles.x << " " << camera_angles.y << " " << camera_angles.z << endl;
        //cout << "Positn: " << cx << " " << cy << " " << cz << endl;
        //cout << "ViewFs: " << v_x << " " << v_y << " " << v_z << endl;

        break;
      default:
        cout << "Non Z_UP not supported yet ;(";
    }


    // Create the good old camera aligned coordinate system.
    gluLookAt(   cx,   cy,   cz,// camera location.
        v_x,  v_y,  v_z,// point looking at.
        up_x, up_y, up_z);// up direction.

    float eyePos[] = {cx, cy, cz};
    glUseProgram(shaderProgID);
    glUniform3fv(glGetUniformLocation(shaderProgID, "eyePos"),
        1, eyePos);
    glUniform1i(glGetUniformLocation(shaderProgID, "envmap"), 1);
      
    //sets ambient, specular, diffuse colors from model
    float colordiffuse[] = {diffuse.r,diffuse.g,diffuse.b};
    float colorspecular[] = {specular.r, specular.g, specular.b};
    float colorambient[] = {ambient.r, ambient.g, ambient.b};

    glUniform3fv(glGetUniformLocation(shaderProgID, "diffuseColor"), 1, colordiffuse);
    glUniform3fv(glGetUniformLocation(shaderProgID, "specularColor"), 1, colorspecular);
    glUniform3fv(glGetUniformLocation(shaderProgID, "ambientColor"), 1, colorambient);
    glUseProgram(0);
  }

  void MeshEdit::draw_meshes()
  {
    for( vector<MeshNode>::iterator n = meshNodes.begin(); n != meshNodes.end(); n++ )
    {
      renderMesh( n->mesh );
    }

    // Execute all of the OpenGL commands.
    glFlush();
  }

  // Guranteed to be called at the start.
  void MeshEdit::resize( size_t w, size_t h)
  {
    screen_w = w;
    screen_h = h;

    // FIX the Text renderer.
    text_mgr.resize(w,h);

    // Enter perspective projection mode.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective( vfov,                  // Field of view in DEGREES.
        1.0*screen_w/screen_h, // Aspect Ratio.
        nearClip,              // distance near side of the view frustrum.
        farClip);              // far side of the view frustrum.

    glUseProgram(outlineShader);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective( vfov,                  // Field of view in DEGREES.
                   1.0*screen_w/screen_h, // Aspect Ratio.
                   nearClip,              // distance near side of the view frustrum.
                   farClip);              // far side of the view frustrum.
    glUseProgram(shaderProgID);
  }

  string MeshEdit::name() {
    return "CelShader";
  }

  string MeshEdit::info() {
    return "Final Project: CelShader";
  }

  void MeshEdit::key_event( char key )
  {
    if(key >= '0' && key <= '9')
    {
        cout << "Reached here" << endl;
      glUseProgram(shaderProgID);
      glUniform1i(glGetUniformLocation(shaderProgID, "outputID"), key-'0');
    }
    switch( key ) {

      // reset view transformation
      case ' ':
        reset_camera();
        break;

      case 'u':
      case 'U':
        mesh_up_sample();
        break;

      case 'i':
      case 'I':
        showHUD = !showHUD;
        break;
      case 'f':
      case 'F':
        flipSelectedEdge();
        break;
      case 'z':
      case 'Z':
        splitSelectedEdge();
        break;
      case 'n':
      case 'N':
        selectNextHalfedge();
        break;
      case 't':
      case 'T':
        selectTwinHalfedge();
        break;
      case 'r':
      case 'R':
        shadingMode = !shadingMode;
        break;
      case 'q':
      case 'Q':
        smoothShading = !smoothShading;
        break;
      case 'w':
      case 'W':
        //printf("%s\n", "move up");
        view_focus += Vector3D(0, 0, -0.5);
        update_camera();
        break;
      case 'a':
      case 'A':
        //printf("%s\n", "move left");

        view_focus += Vector3D(-0.5, 0, 0);
        update_camera();
        break;
      case 's':
      case 'S':
        //printf("%s\n", "move down");
        view_focus += Vector3D(0, 0, 0.5);
        
        update_camera();
        break;
      case 'd':
      case 'D':
        //printf("%s\n", "move right");
        view_focus += Vector3D(0.5, 0, 0);
        update_camera();
        break;
      case '=':
        //printf("%s\n", "move z+");
       //view_focus += Vector3D(0, -1, 0);
        scroll_event( 0, 0.5);
        update_camera();
        break;
      case '-':
        //printf("%s\n", "move z-");
        //view_focus += Vector3D(0, 1, 0);
        scroll_event( 0, -0.5);
        update_camera();

        
      default:
        break;
    }
  }

  void MeshEdit::selectNextHalfedge( void )
  {
    Halfedge* h = NULL;
    if (selectedFeature.isValid()) {
      h = selectedFeature.element->getHalfedge();
    }
    if( h != NULL )
    {
      selectedFeature.element = elementAddress( h->next() );
    }
  }

  void MeshEdit::selectTwinHalfedge( void )
  {
    Halfedge* h = NULL;
    if (selectedFeature.isValid()){
      h = selectedFeature.element->getHalfedge();
    }

    if( h != NULL )
    {
      selectedFeature.element = elementAddress( h->twin() );
    }
  }

  inline float bound(float a, float low, float high)
  {
    return min(high, max(low, a));
  }

  // Mouse event entry point.
  void MeshEdit::cursor_event( float x, float y, unsigned char keys )
  {

    // Mouse dragged if any moust button is held down.
    // if(keys != 0)
    if(left_down || middle_down || right_down)
    {
      mouseD(x, y);
    }
    else // Mouse moved otherwise.
    {
      mouseM(x, y);
    }

    // Always update the correct current mouse coordinates.
    updateMouseCoordinates(x, y);

    return;
  }

  void MeshEdit::scroll_event( float offset_x, float offset_y )
  {

    if(offset_y > 0)
    {
      view_distance -= offset_y*(view_distance/4);
    }

    if(offset_y < 0)
    {
      view_distance -= offset_y*(view_distance/4);
    }

    // Ensure appropiate bounds amounts.
    view_distance = bound(view_distance,
        min_view_distance,
        max_view_distance);

    return;
  }

  void MeshEdit::mouse_button_event( int button, int event )
  {
    switch(event) {
      case MOUSE_BUTTON_RELEASE:
        {
          switch( button )
          {
            case MOUSE_BUTTON_LEFT:
              mouseR( LEFT );
              left_down = false;
              break;
            case MOUSE_BUTTON_RIGHT:
              mouseR( RIGHT );
              right_down = false;
              break;
            case MOUSE_BUTTON_MIDDLE:
              mouseR( MIDDLE );
              middle_down = false;
              break;
          }
          break;
        }
      case MOUSE_BUTTON_PRESS:
        {
          switch( button )
          {
            case MOUSE_BUTTON_LEFT:
              left_down = true;
              mouseP( LEFT );
              break;
            case MOUSE_BUTTON_RIGHT:
              right_down = true;
              mouseP( RIGHT );
              break;
            case MOUSE_BUTTON_MIDDLE:
              middle_down = true;
              mouseP( MIDDLE );
              break;
          }
          break;
        }
    }
  }

  void MeshEdit::load( Scene* scene )
  {
    cout << "MeshEdit: loading scene:\n";

    this->scene = scene;

    // Start out with 0 lights.
    this->light_num = 0;

    std::vector<Node>& nodes = scene->nodes;

    // Iterate through the nodes and initialize the relevant
    // opengl properties.

    int len = nodes.size();
    for(int i = 0; i < len; i++)
    {
      Node & node = nodes[i];

      Instance * instance = node.instance;

      if(!instance)
        continue;

      // FIXME : Transform Matrix?
      switch(instance -> type)
      {
        case CAMERA:
          init_camera(static_cast<Camera&>(*instance));
          break;
        case LIGHT:
          init_light(static_cast<Light&>(*instance));
          break;
        case POLYMESH:
          init_polymesh(static_cast<Polymesh&>(*instance));
          break;
        case MATERIAL:
          init_material(static_cast<Material&>(*instance));
          break;
      }

    }

    cerr << "Done loading scene. Mesh Ready for Editing!" << endl;
  }
  
  Vector3D load_texture(int frame_idx, GLuint handle, const char* where) {
    Vector3D size_retval;
    
    if (strlen(where) == 0) return size_retval;
    
    glActiveTexture(GL_TEXTURE0 + frame_idx);
    glBindTexture(GL_TEXTURE, handle);
    
    
    int img_x, img_y, img_n;
    unsigned char* img_data = stbi_load(where, &img_x, &img_y, &img_n, 3);
    size_retval.x = img_x;
    size_retval.y = img_y;
    size_retval.z = img_n;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_x, img_y, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
    stbi_image_free(img_data);

//    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
//    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    

    // repeat texture
    // visual reference: https://learnopengl.com/Getting-started/Textures
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // how to scale up or down texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    return size_retval;
  }
  
  void MeshEdit::load_textures( void ) {
    glGenTextures(1, &m_gl_texture_1);
//    glGenTextures(1, &m_gl_texture_2);
//    glGenTextures(1, &m_gl_texture_3);
//    glGenTextures(1, &m_gl_texture_4);
//    glGenTextures(1, &m_gl_cubemap_tex);
    
    m_gl_texture_1_size = load_texture(1, m_gl_texture_1, (m_project_root + "/textures/texture_1.png").c_str());
//    m_gl_texture_2_size = load_texture(2, m_gl_texture_2, (m_project_root + "/textures/texture_2.png").c_str());
//    m_gl_texture_3_size = load_texture(3, m_gl_texture_3, (m_project_root + "/textures/texture_3.png").c_str());
//    m_gl_texture_4_size = load_texture(4, m_gl_texture_4, (m_project_root + "/textures/texture_4.png").c_str());

    
    std::cout << "Texture 1 loaded with size: " << m_gl_texture_1_size << std::endl;
//    std::cout << "Texture 2 loaded with size: " << m_gl_texture_2_size << std::endl;
//    std::cout << "Texture 3 loaded with size: " << m_gl_texture_3_size << std::endl;
//    std::cout << "Texture 4 loaded with size: " << m_gl_texture_4_size << std::endl;
    
//    std::vector<std::string> cubemap_fnames = {
//      m_project_root + "/textures/cube/posx.jpg",
//      m_project_root + "/textures/cube/negx.jpg",
//      m_project_root + "/textures/cube/posy.jpg",
//      m_project_root + "/textures/cube/negy.jpg",
//      m_project_root + "/textures/cube/posz.jpg",
//      m_project_root + "/textures/cube/negz.jpg"
//    };
//
//    load_cubemap(5, m_gl_cubemap_tex, cubemap_fnames);
//    std::cout << "Loaded cubemap texture" << std::endl;
  }

  void MeshEdit::init_camera(Camera& camera)
  {
    hfov = camera.hfov;
    vfov = camera.vfov;
    nearClip = camera.nclip;
    farClip  = camera.fclip;
  }

  void MeshEdit::reset_camera()
  {
    view_distance = canonical_view_distance*2.0;
    camera_angles = Vector3D(0., 0., 0.);
    view_focus = original_view_focus;
  }

  void MeshEdit::init_light(Light& light)
  {

    Color & c = light.color;

    // FIXME : 0 - 1.0 or (-1 - 1.0?)
    GLfloat irradiance[] = {c.r, c.g, c.b, c.a};

    // Note: GL_LIGHT i is gurranteed to be equal to GL_LIGHT0 + i.
    GLenum LIGHT_INDEX = GL_LIGHT0 + light_num;
    light_num++;

    /* Enable a single OpenGL light. */
    switch(light.light_type)
    {
      case AMBIENT:
        {
          glLightfv(LIGHT_INDEX, GL_AMBIENT, irradiance);
        }
        break;

      case POINT:
        {
          glLightfv(LIGHT_INDEX, GL_DIFFUSE, irradiance);
          // FIXME : Replace default position.
          const GLfloat arg1[] = {0.0, 0.0, 0.0, 1.0};
          glLightfv(LIGHT_INDEX, GL_POSITION, arg1);
        }
        break;

      case DIRECTIONAL:
        {
          glLightfv(LIGHT_INDEX, GL_DIFFUSE, irradiance);
          // w = 0.0 --> Infinitly far away.
          const GLfloat arg2[] = {1.0, 1.0, 1.0, 0.0};
          glLightfv(LIGHT_INDEX, GL_POSITION, arg2);
          const GLfloat arg3[] = {0.0, 0.0, -1.0};
          glLightfv(LIGHT_INDEX, GL_SPOT_DIRECTION, arg3);
        }

        // direction?
        break;
    }

    // Enable some Opengl parameters.

    // Enable this new openGL light.
    glEnable(LIGHT_INDEX);

  }

  void MeshEdit::init_polymesh( Polymesh& polymesh )
  {
    // Create and store a mesh node object.
    MeshNode meshNode( polymesh );
    meshNodes.push_back( meshNode );

    // Ensure that the current selection always has a valid mesh pointer.
    selectedFeature.node = &meshNode;

    Vector3D low, high;
    meshNode.getBounds( low, high );

    Vector3D centroid;
    meshNode.getCentroid( centroid );
    ambient = polymesh.material->ambi;
    specular = polymesh.material->spec;
    diffuse = polymesh.material->diff;
    
    meshNode.mesh.modelView = Matrix4x4(polymesh.modelView);



    // Determine how far away the camera should be.
    // Minimum distance guaranteed to not clip into the model in C - V.
    canonical_view_distance = (high - low).norm()*1.01; // norm is magnitude.
    min_view_distance = canonical_view_distance / 10.0;
    view_distance     = canonical_view_distance*2.;
    max_view_distance = canonical_view_distance*20.;

    camera_angles = Vector3D(0., 0., 0.);
    view_focus    = centroid ;
    original_view_focus = view_focus;
    //cout << view_focus << endl;
    up = Z_UP;
  }

  void MeshEdit::init_material (Material& material)
  {

    // FIXME : Support Materials.
    return;

  }

  void MeshEdit::mouseP(e_mouse_button b)
  {
    switch (b) {
      case LEFT:
        if(!hoveredFeature.element) {
          mouse_rotate = true;
        }
        else
        {
          enactPotentialSelection();
        }
        break;
      case RIGHT:
        mouse_rotate = true;
        break;
      case MIDDLE:
        break;
    }
  }

  void MeshEdit::mouseR(e_mouse_button b)
  {
    switch (b) {
      case LEFT:
        if(mouse_rotate)
        {
          mouse_rotate = false;
        }
        break;
      case RIGHT:
        mouse_rotate = false;
        break;
      case MIDDLE:
        break;
    }
  }

  void MeshEdit::mouseD(float x, float y)
  {
    // Rotate the camera when the left mouse button is dragged.

    float dx = (x - mouse_x);
    float dy = (y - mouse_y);

    Vertex* v = NULL;
    if (selectedFeature.isValid()) {
      v = selectedFeature.element->getVertex();
      cout << v;

    }

    if(!mouse_rotate && v != NULL)
    {
      dragPosition(dx, dy, v->position);
      return;
    }


    if(mouse_rotate)
    {
      
      /*double & cx = camera_angles.x;
      double & cy = camera_angles.y;

      double alpha_x = dx * 2 * PI / screen_w;
      double alpha_y = dy * PI / screen_h;

      double vector_x_0[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
      double vector_x_1[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
      double vector_x_2[] = {0.0,-1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0};

      Vector4D initial_view_focus = Vector4D(view_focus.x, view_focus.y, view_focus.z, 1);

      double t_data[] = {
        1.0, 0.0, 0.0, -initial_view_focus.x,
        0.0, 1.0, 0.0, -initial_view_focus.y,
        0.0, 0.0, 1.0, -initial_view_focus.z,
        0.0, 0.0, 0.0, 1.0,
      };
      double t1_data[] = {
        1.0, 0.0, 0.0, initial_view_focus.x,
        0.0, 1.0, 0.0, initial_view_focus.y,
        0.0, 0.0, 1.0, initial_view_focus.z,
        0.0, 0.0, 0.0, 1.0,
      };
      double rz_data[] = {
        cos(alpha_x),-sin(alpha_x), 0.0, 0.0,
        sin(alpha_x), cos(alpha_x), 0.0, 0.0,
                 0.0,          0.0, 1.0, 0.0,
                 0.0,          0.0, 0.0, 1.0,
      };

      Matrix4x4 T = Matrix4x4(t_data);
      Matrix4x4 T1 = Matrix4x4(t1_data);
      Matrix4x4 Rz = Matrix4x4(rz_data);

      Vector4D x_transform_view_focus = T1 * Rz * T * initial_view_focus;
      view_focus.x = x_transform_view_focus.x;
      view_focus.y = x_transform_view_focus.y;
      view_focus.z = x_transform_view_focus.z;

      cx += alpha_x;
      // Matrix3x3 x_transform = cos(alpha_x) * Matrix3x3(vector_x_0);
      // x_transform += (1.0 - cos(alpha_x)) * Matrix3x3(vector_x_1);
      // x_transform += sin(alpha_x) * Matrix3x3(vector_x_2);

      // cout << x_transform << endl;
      // view_focus = x_transform * view_focus;

      // cy += alpha_y;


      // Users can freely rotate the model's as much as they
      // want in the horizontal direction.
      if (cx >= 2 * PI) {
        cx -= 2 * PI;
      } else if (cx < 0) {
        cx += 2 * PI;
      }
      // cx = cx >= 0 ? min(cx, cx - 2*PI) : (cx + 2*PI);

      // Bound the vertical view angle.
      camera_angles.y = bound(camera_angles.y, -PI/2, PI/2);*/

      double & cx = camera_angles.x;

      double & cy = camera_angles.y;

      cx += dx*2*PI/screen_w;
      cy += dy*  PI/screen_h;

      // Users can freely rotate the model's as much as they
      // want in the horizontal direction.
      cx = cx >= 0 ? min(cx, cx - 2*PI) : (cx + 2*PI);

      // Bound the vertical view angle.
      camera_angles.y = bound(camera_angles.y, -PI/2, PI/2);
      
    }
  }

  void MeshEdit::mouseM(float x, float y)
  {
    // Highlight the mesh element the mouse is hovering over.
    findMouseSelection(x, y);
  }

  void MeshEdit::updateMouseCoordinates(float x, float y)
  {
    mouse_x = x;
    mouse_y = y;
  }

  // FIXME : Replace with standard Vector4D library call.
  inline Vector2D to2DVector(Vector4D & V)
  {
    return Vector2D(V.x, V.y);
  }

  // Returns true iff the given point is inside of this triangle.
  // Stores barycentric coordinates in A if return is true.
  inline bool point_in_triangle_and_barycentric_coordinates(
      Vector2D & A, Vector2D & B, Vector2D & C, Vector2D point)
  {

    // Compute vectors
    Vector2D v0 = C - A;
    Vector2D v1 = B - A;
    Vector2D v2 = point - A;

    // Compute dot products
    double dot00 = dot(v0, v0);
    double dot01 = dot(v0, v1);
    double dot02 = dot(v0, v2);
    double dot11 = dot(v1, v1);
    double dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Check if point is in triangle.
    bool output = (u >= 0) && (v >= 0) && (u + v < 1);

    if(output)
    {
      A.x = u;
      A.y = v;
    }

    return output;
  }

  inline Vector2D MeshEdit::unitCubeToScreenSpace(Vector4D & in)
  {
    return Vector2D(screen_w*(in.x + 1)/2, screen_h*(in.y + 1)/2);
  }

  // IN :
  //    selectionPoint --- coordinates of the cursor in screen space.
  //    A,B,C --- vertex coordinates of the triangle in model space.
  //
  // IN/OUT :
  //
  //    w --- The value of w passed in is the smallest w value seen so far.
  //    The method will return false if the current triangle is further away than this value (greater w value ) -OR- if
  //    the point selectionPoint is not inside the triangle ABC, projected onto screen space.
  //    Otherwise, it updates the w value with the w value of the current triangle (which is closer than anything seen before).
  //
  // OUT :
  //    barycentricCoordinates --- contains the screen space barycentric coordinates of selectionPoint.
  //
  inline bool MeshEdit::triangleSelectionTest(
      const Vector2D & selectionPoint,
      Vector4D & A, Vector4D & B, Vector4D & C,
      float & w,
      Vector3D& barycentricCoordinates )
  {
    /*
     * Algorithm for computing from model space coordinates X to
     * screen space with a depth value using opengl.
     *
     * Start with Vector X.
     * 1. Get P = opengl projection Matrix.
     *    Get M = opengl Model transform matrix.
     *
     * 2. Convert into our personal 462 library matrix representation.
     *
     * 3. Model to world space: X = M*X;
     *
     * 4. World to homogenous space X = P*X;
     *
     * 5. Projection divide.(homogenesous space to 3D unit cube space.)
     *
     *  - X.x = X.x/X.w;
     *  - X.y = X.y/X.w;
     *  - X.z = X.z/X.w;
     *
     * x, y, and z are now in the range [-1, 1].
     *
     * 6. Unit cube space to screen space.
     * screen_x = viewport.x + viewport.width  * (X.x +1)/2;
     * screen_y = viewport.y + viewport.height * (X.y +1)/2;
     *
     * In our case, the input screen coordinates are already in
     * window space, so we can omit the viewport offset values.
     *
     * 7. Note that opengl coordinate shceme is from lower left corner.
     */

    // -- Step 1, extract relevant opengl matrices.

    GLdouble projMatrix[16];
    GLdouble modelMatrix[16];

    for(int i = 0; i < 16; i++)
    {
      projMatrix[i]  = 0.0;
      modelMatrix[i] = 0.0;
    }

    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetDoublev(GL_MODELVIEW_MATRIX,  modelMatrix);


    Matrix4x4 P;
    Matrix4x4 M;

    for(int r = 0; r < 4; r++)
      for(int c = 0; c < 4; c++)
      {
        P(r, c) = projMatrix [4*c + r];
        M(r, c) = modelMatrix[4*c + r];
      }

    // -- Step 3 & 4. Apply Model transform, then Projection transform.

    A = P*M*A;
    B = P*M*B;
    C = P*M*C;

    // -- Step 5. Projection divide.
    A /= A.w;
    B /= B.w;
    C /= C.w;

    // -- Step 6.  Unit cube space to screen space.
    /* screen_x = viewport.x + viewport.width  * (X.x +1)/2;
     * screen_y = viewport.y + viewport.height * (X.y +1)/2;
     */

    Vector2D A_out = unitCubeToScreenSpace(A);
    Vector2D b_2d  = unitCubeToScreenSpace(B);
    Vector2D c_2d  = unitCubeToScreenSpace(C);
    bool inside = point_in_triangle_and_barycentric_coordinates(A_out, b_2d, c_2d, selectionPoint);

    // If inside, then we need to go and compute the meta data.
    if(inside)
    {
      // Percentage towards C.
      float bary_u = A_out.x;// u

      // Percentage towards B.
      float bary_v = A_out.y;// v

      // During the projection the z component becomes indicative of depth.
      /*A.w = A.z;
        B.w = B.z;
        C.w = C.z;
       */

      // Change in u over 1 u.
      float dwu = C.w - A.w;
      // Change in v over 1 v.
      float dwv = B.w - A.w;

      /* Interpolate Along the linear plane in the plane aligned
       * coordinate system.
       */
      float w_new = A.w + dwu*bary_u + dwv*bary_v;


      /* Determine whether this triangle is closer to the viewer within
       * the view frustrum using its homogeneous w coordinate,
       * which is NOT the same as its z coordinate.
       * NOTE: w < 0.0 implies no previous satisfactory triangle
       *       has been seen.
       */
      if(w < 0.0 || (w_new > 0 && w_new < w))
      {

        // VERY IMPORTANT! We need to update the minnumum.
        w = w_new;

        barycentricCoordinates.x = 1.0 - bary_u - bary_v;// percentage towards A.
        barycentricCoordinates.y = bary_v;// % B. (=1 --> on B, =0 --> on opposite edge from B.
        barycentricCoordinates.z = bary_u;// % C
        return true;
      }
    }

    // The point is not inside of the triangle,
    // or the triangle is not at a good depth.
    return false;
  }

  // Picking algorithm entry point.
  // Linear in the number of triangles in the scene.
  void MeshEdit::findMouseSelection(float x, float y)
  {
    if(shadingMode)
    {
      selectedFeature.invalidate();
      hoveredFeature.invalidate();
      return;
    }

    bool foundSelection = false; // Will be true if and only if we find a selection.

    MeshFeature closestFeature;
    MeshFeature currentFeature;
    MeshNode* closestNode;

    Vector4D A, B, C;
    Vector3D barycentric_min;

    /*
     * IMPORTANT NOTE: OpenGL coordinate system orgin at bottom left of
     * screen. Y points up, so we need to flip y by screen_h - y.
     */
    const Vector2D selectionPoint = Vector2D( x, screen_h - y );

    // Start out behind the camera.
    float w = -1.0;

    // Iterate through all meshes.
    int num_meshes = meshNodes.size();
    for(int mesh_index = 0; mesh_index < num_meshes; mesh_index++)
    {
      MeshNode& node = meshNodes[mesh_index];

      // Iterate through all triangles.
      for( FaceIter f = node.mesh.facesBegin(); f != node.mesh.facesEnd(); f++ )
      {
        // Build a mesh feature corresponding to the current face.
        currentFeature.element = elementAddress( f );
        currentFeature.node = &node;

        // Copy the three vertex coordinates of the face into 4D homogeneous coordinates.
        A = Vector4D( f->halfedge()->vertex()->position );
        B = Vector4D( f->halfedge()->next()->vertex()->position );
        C = Vector4D( f->halfedge()->next()->next()->vertex()->position );
        A.w = B.w = C.w = 1.;

        Vector3D barycentricCoordinates;
        if( triangleSelectionTest( selectionPoint, A, B, C, w, barycentricCoordinates ) )
          // If the cursor is inside triangle ABC --AND-- this triangle is closer to the viewer
          // than anything we've seen so far, we'll update the record of the closest feature
          // we've seen so far.
        {
          // Update the record of the closest feature seen so far; note that the value of w
          // was already updated in our call to triangleSelectionTest.
          barycentric_min = barycentricCoordinates;
          closestFeature = currentFeature;
          closestNode = &node;

          foundSelection = true;
        }

      } // Done iterating through all triangles.

    } // Done iterating over meshes.

    // Update the Current hoveredFeature values.
    if( foundSelection )
    {
      closestFeature.node->fillFeatureStructure( this->hoveredFeature, closestFeature, barycentric_min, w );
      hoveredFeature.node = closestNode;
    }
    else // If the cursor is not hovering over any element, clear the selection.
    {
      this->hoveredFeature.invalidate();
    }
  }

  // Copies 'hoveredFeature' to 'selectedFeature'.
  void MeshEdit::enactPotentialSelection()
  {
    selectedFeature = hoveredFeature;
  }

  // Transforms the position vector in world space according to an offset in screenspace.
  void MeshEdit::dragPosition(float screen_x_offset, float screen_y_offset,
      Vector3D & position)
  {

    GLdouble projMatrix[16];
    GLdouble modelMatrix[16];

    for(int i = 0; i < 16; i++)
    {
      projMatrix[i]  = 0.0;
      modelMatrix[i] = 0.0;
    }

    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetDoublev(GL_MODELVIEW_MATRIX,  modelMatrix);


    Matrix4x4 P;
    Matrix4x4 M;

    for(int r = 0; r < 4; r++)
      for(int c = 0; c < 4; c++)
      {
        P(r, c) = projMatrix [4*c + r];
        M(r, c) = modelMatrix[4*c + r];
      }

    Vector4D pos = Vector4D(position);
    pos.w = 1.0;

    // Project into 3d Homogeneous coordinates.
    pos = P*M*pos;

    // -- Step 5. Projection divide into unit cube.
    pos /= pos.w;


    // Compute the offset vector in normalized screen space coordinates.
    Vector4D screen_offset(screen_x_offset*2/screen_w,
        -screen_y_offset*2/screen_h, 0.0, 0.0);

    pos += screen_offset;


    // Lets go back the way we came.
    pos *= pos.w;

    pos = M.inv()*P.inv()*pos;

    position = pos.to3D();


  }

  // -- Geometric Operations
  void MeshEdit::mesh_up_sample()
  {
    HalfedgeMesh* mesh;

    // If an element is selected, resample the mesh containing that
    // element; otherwise, resample the first mesh in the scene.
    if( selectedFeature.isValid() )
    {
      mesh = &( selectedFeature.node->mesh );
    }
    else
    {
      mesh = &( meshNodes.begin()->mesh );
    }

    resampler.upsample( *mesh );

    // Since the mesh may have changed, the selected and
    // hovered features may no longer point to valid elements.
    selectedFeature.invalidate();
    hoveredFeature.invalidate();
  }

  inline void MeshEdit::drawString(float x, float y, string str, size_t size, Color c)
  {
    int line_index = text_mgr.add_line(( x*2/screen_w) - 1.0,
        (-y*2/screen_h) + 1.0, str, size, c);
    messages.push_back(line_index);
  }

  /*
   * You can modify the print statements in this function to control the
   * debugging information that you wish to print.
   * Depending on how far along you are various properties will segfault
   * if your mesh does not maintain its invariants.
   */
  // FIXME : Convert these to messages on screen with SKY's code.
  void MeshEdit::drawHUD()
  {

    // Delete the current Lines every time.
    int len = messages.size();
    for(int i = 0; i < len; i++)
    {
      int line_index = messages[i];
      text_mgr.del_line(line_index);
    }
    messages.clear();


    const size_t size = 16;
    const float x0 = use_hdpi ? screen_w - 350 * 2 : screen_w - 350;
    const float y0 = use_hdpi ? 128 : 64;
    const int inc  = use_hdpi ? 48  : 24;
    float y = y0 + inc - size;

    // No selection --> no messages.
    if(!selectedFeature.isValid())
    {
      ostringstream m1;
      m1 << "No Mesh Feature is selected.";

      drawString(x0, y, m1.str(), size, text_color);y += inc;

    } else {

      Vertex *v = selectedFeature.element->getVertex();
      if (v != NULL) {
        ostringstream m1, m2, m3, m4, m5, m6, m7, m8;
        m1 << "VERTEX DATA";
        m2 << "address      = " << v;

        // -- Nicely format position data.
        Vector3D &pos = v->position;
        m3 << "position:  x = ";

        // -- X.
        m3 << scientific;
        m3.precision(4);
        m3 << pos.x;

        // -- Y.
        m4 << scientific;
        m4.precision(4);
        m4 << "           y = " << pos.y;

        // -- Z.
        m5 << scientific;
        m5.precision(4);
        m5 << "           z = " << pos.z;


        m6 << "halfedge()   = " << elementAddress(v->halfedge());
        m7 << "isBoundary() = " << v->isBoundary();
        m8 << "degree()     = " << v->degree();

        drawString(x0, y, m1.str(), size, text_color);
        y += inc;
        drawString(x0, y, m2.str(), size, text_color);
        y += inc;
        y += inc;// linebreak after address.
        drawString(x0, y, m3.str(), size, text_color);
        y += inc;
        drawString(x0, y, m4.str(), size, text_color);
        y += inc;
        drawString(x0, y, m5.str(), size, text_color);
        y += inc;
        y += inc; // linebreak after position.
        drawString(x0, y, m6.str(), size, text_color);
        y += inc;
        drawString(x0, y, m7.str(), size, text_color);
        y += inc;
        drawString(x0, y, m8.str(), size, text_color);
        y += inc;
      }

      Halfedge *h = selectedFeature.element->getHalfedge();
      if (h != NULL) {

        ostringstream m1, m2, m3, m4, m5, m6, m7, m8;

        m1 << "HALFEDGE DATA";
        m2 << "address      = " << h;
        m3 << "twin()       = " << elementAddress(h->twin());
        m4 << "next()       = " << elementAddress(h->next());
        m5 << "vertex()     = " << elementAddress(h->vertex());
        m6 << "edge()       = " << elementAddress(h->edge());
        m7 << "face()       = " << elementAddress(h->face());
        m8 << "isBoundary() = " << h->isBoundary() << endl;


        drawString(x0, y, m1.str(), size, text_color);
        y += inc;
        drawString(x0, y, m2.str(), size, text_color);
        y += inc;
        y += inc;
        drawString(x0, y, m3.str(), size, text_color);
        y += inc;
        drawString(x0, y, m4.str(), size, text_color);
        y += inc;
        drawString(x0, y, m5.str(), size, text_color);
        y += inc;
        drawString(x0, y, m6.str(), size, text_color);
        y += inc;
        drawString(x0, y, m7.str(), size, text_color);
        y += inc;
        drawString(x0, y, m8.str(), size, text_color);
        y += inc;

      }

      Edge *e = selectedFeature.element->getEdge();
      if (e != NULL) {
        ostringstream m1, m2, m3, m4;

        m1 << "EDGE DATA";
        m2 << "address      = " << e;
        m3 << "halfedge()   = " << elementAddress(e->halfedge());
        m4 << "isBoundary() = " << e->isBoundary() << endl;

        drawString(x0, y, m1.str(), size, text_color);
        y += inc;
        drawString(x0, y, m2.str(), size, text_color);
        y += inc;
        y += inc;
        drawString(x0, y, m3.str(), size, text_color);
        y += inc;
        drawString(x0, y, m4.str(), size, text_color);
        y += inc;
      }


      Face *f = selectedFeature.element->getFace();
      if (f != NULL) {
        ostringstream m1, m2, m3, m4, m5;//, m6, m7, m8;

        m1 << "FACE DATA";
        m2 << "     address = " << f << endl;
        m3 << "  halfedge() = " << elementAddress(f->halfedge()) << endl;
        m4 << "    degree() = " << f->degree() << endl;
        m5 << "isBoundary() = " << f->isBoundary() << endl;

        drawString(x0, y, m1.str(), size, text_color);
        y += inc;
        drawString(x0, y, m2.str(), size, text_color);
        y += inc;
        y += inc;
        drawString(x0, y, m3.str(), size, text_color);
        y += inc;
        drawString(x0, y, m4.str(), size, text_color);
        y += inc;
        drawString(x0, y, m5.str(), size, text_color);
        y += inc;

      }
    }

    // -- First draw a lovely black rectangle.

    glPushAttrib( GL_VIEWPORT_BIT );
    glViewport( 0, 0, screen_w, screen_h );

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, screen_w, screen_h, 0, 0, 1 ); // Y flipped !

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glTranslatef( 0, 0, -1 );


    // -- Black with opacity .8;

    glColor4f(0.0, 0.0, 0.0, 0.8);

    float min_x = x0 - 32;
    float min_y = y0 - 32;
    float max_x = screen_w;
    float max_y = y;

    float z = 0.0;


    glDisable(GL_DEPTH_TEST);


    glBegin(GL_QUADS);

    glVertex3f(min_x, min_y, z);
    glVertex3f(min_x, max_y, z);
    glVertex3f(max_x, max_y, z);
    glVertex3f(max_x, min_y, z);
    glEnd();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glPopAttrib();

    glEnable( GL_DEPTH_TEST );

    text_mgr.render();


  }

  //===================== End of MeshEdit class.


  //************************************************************************/
  // ----------------------- Mesh Node functions. --------------------------/
  //  **********************************************************************/

  inline void setColor(Color & c)
  {
    // FIXME? : Incorporate antialiasing.
    glColor3f(c.r, c.g, c.b);
  }

  void MeshEdit::renderMesh( HalfedgeMesh& mesh )
  {
    Matrix4x4 mat = mesh.modelView;
    // draw them outlines
    glUseProgram(outlineShader);
    outlineGLSettings();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
//    glLoadIdentity();
    glTranslated( mat(0,3), mat(1,3), 0.f );
    drawFaces(mesh, true);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    mainGLSettings();
    
    if(shadingMode)
      glUseProgram(shaderProgID);
    else
      glUseProgram(0);
    glTranslated( mat(0,3), mat(1,3), mat(2,3) );
    glEnable(GL_LIGHTING);
    drawFaces( mesh, false );
    glDisable(GL_LIGHTING);

    glUseProgram(0);

    if(!shadingMode)
    {
      // Edges are drawn with flat shading.
      drawEdges( mesh );

      drawVertices( mesh );
      drawHalfedges( mesh );
    }
  }

  // Sets the current OpenGL color/style of a given mesh element, according to which elements are currently selected and hovered.
  inline void MeshEdit::setElementStyle( HalfedgeElement* element )
  {
    // Set the draw style according to whether the
    // current mesh element is hovered or selected,
    // giving priority to selection.
    DrawStyle* style = &defaultStyle;
    if( element == selectedFeature.element )
    {
      style = &selectStyle;
    }
    else if( element == hoveredFeature.element )
    {
      style = &hoverStyle;
    }

    // Now set draw attributes according to the type of mesh element.
    if( element->getFace()     ) { setColor( style->faceColor     );                                     return; }
    if( element->getEdge()     ) { setColor( style->edgeColor     ); glLineWidth( style->strokeWidth  ); return; }
    if( element->getHalfedge() ) { setColor( style->halfedgeColor ); glLineWidth( style->strokeWidth  ); return; }
    if( element->getVertex()   ) { setColor( style->vertexColor   ); glPointSize( style->vertexRadius ); return; }

    cerr << "Warning: draw style not defined for current mesh element!" << endl;
  }

  void MeshEdit::drawFaces( HalfedgeMesh& mesh, bool noDetail )
  {
    // pass variables into shader
    if(!noDetail) {
      // textures
      glUniform1i(glGetUniformLocation(shaderProgID, "u_texture_1"), 1);
    }
    for( FaceIter f = mesh.facesBegin(); f != mesh.facesEnd(); f++ )
    {

      // These guys prevent z fighting / prevents the faces from bleeding into the edge lines and points.
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset( 1.0, 1.0 );

      glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);

      // Coloring.
      setElementStyle( elementAddress( f ) );

      // Start specifying the polygon.
      glBegin(GL_POLYGON);

      // Set the normal of this face.
      Vector3D normal = f->normal();

      glNormal3dv( &normal.x );

      // iterate over this polygon's vertices
      HalfedgeIter h = f->halfedge();
      do
      {
        if(smoothShading)
          normal = h->vertex()->normal();
        glNormal3dv( &normal.x );
          
        

        
        // draw them tex coords
        Vector2D texcoord = h->vertex()->texcoord;

        // if no texcoords defined, use vertices
        if(texcoord.x < 0 || texcoord.y < 0) {
          glTexCoord3dv(&h->vertex()->position.x);
        } else { // otherwise use defined texcoords
          // 1.0 - stuff because Blender and OpenGL use left hand and right hand coordinates respectively
          glTexCoord2d(texcoord.x, 1.0 - texcoord.y);
        }
      
        // Draw this vertex.
        Vector3D position = h->vertex()->position;
        glVertex3dv( &position.x );

        // go to the next vertex in this polygon
        h = h->next();

      } while( h != f->halfedge() ); // end of iteration over polygon vertices

      // Finish drawing the polygon.
      glEnd();

    }// End of per polygon loop.

  }

  void MeshEdit::drawEdges( HalfedgeMesh& mesh )
  {
    for( EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++ ) // iterate over edges
    {
      Vector3D p0 = e->halfedge()->vertex()->position;
      Vector3D p1 = e->halfedge()->twin()->vertex()->position;

      setElementStyle( elementAddress( e ) );

      glBegin(GL_LINES);
      glVertex3dv( &p0.x );
      glVertex3dv( &p1.x );
      glEnd();

    } // done iterating over edges
  }

  void MeshEdit::drawVertices( HalfedgeMesh& mesh )
  {
    Vertex* v;

    glDisable(GL_DEPTH_TEST);


    // Draw the hover vertex
    if (hoveredFeature.isValid()) {
      v = hoveredFeature.element->getVertex();
      if( v != NULL )
      {
        setElementStyle( v );

        glBegin( GL_POINTS );
        Vector3D p = v->position;
        glVertex3d( p.x, p.y, p.z );
        glEnd();
      }
    }


    // Draw the selected vertex.
    if (selectedFeature.isValid()) {
      v = selectedFeature.element->getVertex();
      if( v != NULL )
      {
        setElementStyle( v );

        glBegin( GL_POINTS );
        Vector3D p = v->position;
        glVertex3d( p.x, p.y, p.z );
        glEnd();
      }
    }


    glEnable( GL_DEPTH_TEST );
  }

  void MeshEdit::drawHalfedgeArrow( Halfedge* h )
  {
    setElementStyle( h );

    Vector3D p0 = h->vertex()->position;
    Vector3D p1 = h->next()->vertex()->position;
    Vector3D p2 = h->next()->next()->vertex()->position;

    Vector3D N = h->face()->normal();

    Vector3D e01 = p1-p0;
    Vector3D e12 = p2-p1;
    Vector3D e20 = p0-p2;

    Vector3D u = (e01-e20)/2.;
    Vector3D v = (e12-e01)/2.;

    Vector3D a = p0 + .2*u;
    Vector3D b = p1 + .2*v;

    Vector3D s = .2*(b-a);
    Vector3D t = cross( N, s );
    double theta = M_PI-M_PI/6.;
    Vector3D c = b + cos(theta)*s + sin(theta)*t;

    glBegin( GL_LINE_STRIP );
    glVertex3dv( &a.x );
    glVertex3dv( &b.x );
    glVertex3dv( &c.x );
    glEnd();
  }


  void MeshEdit::drawHalfedges( HalfedgeMesh& mesh )
  {
    Halfedge* h;

    glDisable(GL_DEPTH_TEST);

    if (hoveredFeature.isValid()) {
      h = hoveredFeature.element->getHalfedge();
      if (h != NULL) {
        drawHalfedgeArrow(h);
      }
    }

    if (selectedFeature.isValid()) {
      h = selectedFeature.element->getHalfedge();
      if (h != NULL) {
        drawHalfedgeArrow(h);
      }
    }

    glEnable( GL_DEPTH_TEST );
  }

  void MeshNode::getBounds( Vector3D& low, Vector3D& high )
  {
    double maxValue = numeric_limits<double>::max();

    low.x = maxValue; high.x = -maxValue;
    low.y = maxValue; high.y = -maxValue;
    low.z = maxValue; high.z = -maxValue;

    for( VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++ )
    {
      Vector3D& p = v->position;

      low.x = min( low.x, p.x );
      low.y = min( low.y, p.y );
      low.z = min( low.z, p.z );

      high.x = max( high.x, p.x );
      high.y = max( high.y, p.y );
      high.z = max( high.z, p.z );
    }
  }

  // Centroid / weighted average point.
  void MeshNode::getCentroid( Vector3D& centroid )
  {
    centroid = Vector3D( 0., 0., 0. );

    for( VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++ )
    {
      centroid += v->position;
    }

    centroid /= (double) mesh.nVertices();
  }

  /*
   * populates the given feature structure with data cooresponding to
   * mesh feature on the face cooresponding to the given lookup structure
   * and bary_centric_coordinates.
   * OUT : feature.
   * IN : Lookup location : Which triangular face.
   * barycentric_coordates : Where on the face is the mouse pointing to.
   *  - Determines whether the user wants the face, an edge, or a vertex.
   *  - Vector(A%, B%, C%), where ABC are in index order after the lookup location.
   */
  void MeshNode::fillFeatureStructure(
      // OUTPUT:
      MeshFeature & feature,

      // INPUTS:
      MeshFeature & lookup,
      Vector3D    & barycentric_coords,
      float w )
  {
    /* Feature on Face Selection algorithm.
     *
     * coordinate c is low  if (c < low_threshold)
     * coordinate c is mid  if (low_threshold <= low_threshold <= high_hold)
     * coordinate c is high if (c > high_threshold)
     */

    Face* f = lookup.element->getFace();
    if( f == NULL )
    {
      cerr << "Error in Mesh::fillFeatureStructure(): we were asked to find a feature associated with an element that is not a face!" << endl;
      exit( 1 );
    }

    // Grab the three halfedges of the triangle under the cursor.
    HalfedgeIter h1 = f->halfedge();
    HalfedgeIter h2 = h1->next();
    HalfedgeIter h3 = h2->next();

    // Grab the three "root" vertices of these halfedges.
    VertexIter v1 = h1->vertex();
    VertexIter v2 = h2->vertex();
    VertexIter v3 = h3->vertex();

    // The output feature will keep track of the mesh the element comes from,
    // as well as the depth coordinate associated with the current cursor location.
    feature.node = this;
    feature.w = w;

    // Check if the cursor is closest to a vertex; if so, this is the feature we want to return.
    if( barycentric_coords.x > high_threshold ) { feature.element = elementAddress( v1 ); return; }
    if( barycentric_coords.y > high_threshold ) { feature.element = elementAddress( v2 ); return; }
    if( barycentric_coords.z > high_threshold ) { feature.element = elementAddress( v3 ); return; }

    // Next, check if the cursor is closest to an edge; if so, we return it.
    if( barycentric_coords.z < low_threshold) { feature.element = elementAddress( h1->edge() ); return; }
    if( barycentric_coords.x < low_threshold) { feature.element = elementAddress( h2->edge() ); return; }
    if( barycentric_coords.y < low_threshold) { feature.element = elementAddress( h3->edge() ); return; }

    // Finally, check if the cursor is closest to a halfedge; if so, we return the associated halfedge.
    if( barycentric_coords.z < mid_threshold) { feature.element = elementAddress( h1 ); return; }
    if( barycentric_coords.x < mid_threshold) { feature.element = elementAddress( h2 ); return; }
    if( barycentric_coords.y < mid_threshold) { feature.element = elementAddress( h3 ); return; }

    // Otherwise, the cursor is closest to the (middle of) the face itself.
    feature.element = f;
    return;
  }

  void MeshEdit :: flipSelectedEdge( void )
  {
    Edge* e = NULL;
    if (selectedFeature.isValid()){
      e = selectedFeature.element->getEdge();
    }

    if( e == NULL ) { cerr << "Must select an edge." << endl; return; }
    selectedFeature.node->mesh.flipEdge( e->halfedge()->edge() );

    // Since the mesh may have changed, the selected and
    // hovered features may no longer point to valid elements.
    selectedFeature.invalidate();
    hoveredFeature.invalidate();
  }

  void MeshEdit :: splitSelectedEdge( void )
  {
    Edge* e = NULL;
    if (selectedFeature.isValid()) {
      e = selectedFeature.element->getEdge();
    }

    if( e == NULL ) { cerr << "Must select an edge." << endl; return; }
    selectedFeature.node->mesh.splitEdge( e->halfedge()->edge() );

    // Since the mesh may have changed, the selected and
    // hovered features may no longer point to valid elements.
    selectedFeature.invalidate();
    hoveredFeature.invalidate();
  }
} // namespace CMU462
