#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/NGLStream.h>
#include <ngl/Util.h>



NGLScene::NGLScene()
{
  setTitle("Shift Click to place ");
}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}




void NGLScene::resizeGL(int _w , int _h)
{
  m_win.width=_w*devicePixelRatio();
  m_win.height=_h*devicePixelRatio();
  m_projection=ngl::perspective(45,float(width()/height()),0.5f,50.0f);
  m_text->setScreenSize(m_win.width,m_win.height);
}


void NGLScene::initializeGL()
{
  // we need to initialise the NGL lib which will load all of the OpenGL functions, this must
  // be done once we have a valid GL context but before we call any GL commands. If we dont do
  // this everything will crash
  ngl::NGLInit::instance();
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
   // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  shader->setUniform("Colour",1.0f,1.0f,1.0f,1.0f);
  shader->setUniform("lightPos",1.0f,1.0f,1.0f);
  shader->setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);

  ngl::Vec3 from(0,0,4);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  m_view=ngl::lookAt(from,to,up);


  m_projection=ngl::perspective(45,float(width()/height()),0.5f,50.0f);
  m_text.reset(new ngl::Text(QFont("Ariel",24)));
  m_text->setColour(1.0f,1.0f,1.0f);

}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_transform.getMatrix();
  MV=  m_view*M;
  MVP= m_projection*MV;
  normalMatrix=MV;
  normalMatrix.inverse().transpose();
  shader->setUniform("MVP",MVP);
  shader->setUniform("normalMatrix",normalMatrix);
 }

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_win.width,m_win.height);
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_win.spinXFace);
  rotY.rotateY(m_win.spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  // draw

  for(unsigned int i=0; i<m_positions.size(); ++i)
  {
    //m_transform.reset();
    m_transform.setPosition(m_positions[i]);
    loadMatricesToShader();
    prim->draw("cube");
  }
  if(m_positions.size() !=0)
  {
  ngl::Vec3 last=m_positions.back();

  QString text=QString("Pos=(%1,%2) World=(%3,%4,%5)")
      .arg(m_screenClick.m_x)
      .arg(m_screenClick.m_y)
      .arg(last.m_x)
      .arg(last.m_y)
      .arg(last.m_z);
   m_text->renderText(10,10,text);
  }

}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_win.rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_win.origX;
    int diffy=_event->y()-m_win.origY;
    m_win.spinXFace += 0.5f * diffy;
    m_win.spinYFace += 0.5f * diffx;
    m_win.origX = _event->x();
    m_win.origY = _event->y();
    update();

  }
        // right mouse translate code
  else if(m_win.translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = _event->x() - m_win.origXPos;
    int diffY = _event->y() - m_win.origYPos;
    m_win.origXPos=_event->x();
    m_win.origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true


  if (_event->modifiers() == Qt::ShiftModifier && _event->button() == Qt::LeftButton)
  {
    m_screenClick.set(_event->x(),_event->y());
    m_positions.push_back( getWorldSpace(_event->x(),_event->y()) );
    update();
  }
  else if(_event->button() == Qt::LeftButton)
  {
    m_win.origX = _event->x();
    m_win.origY = _event->y();
    m_win.rotate =true;
  }

  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_win.origXPos = _event->x();
    m_win.origYPos = _event->y();
    m_win.translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_win.rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_win.translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_Space : m_positions.clear(); break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}

ngl::Vec3 NGLScene::getWorldSpace(int _x, int _y)
{
  std::cout<<"Mouse pos "<<_x<<" "<<_y<<" ";
/*  ngl::Mat4 t=m_projection;
  ngl::Mat4 v=m_view;
  // as ngl:: and OpenGL use different formats need to transpose the matrix.
  ngl::Mat4 inverse=( t*v).inverse();

  ngl::Vec4 tmp(0,0,1.0f,1.0f);
  // convert into NDC
  tmp.m_x=(2.0f * _x) / width() - 1.0f;
  tmp.m_y=1.0f - (2.0f * _y) / height();
  // scale by inverse MV * Project transform
  ngl::Vec4 obj=inverse*tmp;
  // Scale by w
  obj/=obj.m_w;
  return obj.toVec3();*/
   //ngl now has this built in as well
 return ngl::unProject(ngl::Vec3(_x,_y,1.0f),
                        m_view,
                        m_projection,
                        ngl::Vec4(0,0,width(),height())
                        );

}
