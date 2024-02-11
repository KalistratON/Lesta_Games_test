
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#include <cassert>
#include <vector>
#include <algorithm>
#include <cmath>

#include "scene.hpp"


namespace Scene
{
	namespace
	{
		namespace View
		{
			constexpr float width = 16.f;
			constexpr float height = 9.f;
		}

		constexpr float pi = 3.14159265f;


		enum class Color
		{
			red,
			green,
			blue,
			black,
			white
		};


		void setupGLColor( Color color )
		{
			switch ( color )
			{
				case Color::red:
					glColor3f( 1.f, 0.f, 0.f );
					break;
				case Color::green:
					glColor3f( 0.f, 1.f, 0.f );
					break;
				case Color::blue:
					glColor3f( 0.f, 0.f, 1.f );
					break;
				case Color::black:
					glColor3f( 0.f, 0.f, 0.f );
					break;
				case Color::white:
					glColor3f( 1.f, 1.f, 1.f );
					break;
			}
		}
	}
}


//-------------------------------------------------------
//	user interface: common mesh support
//-------------------------------------------------------

namespace Scene
{
	class Mesh
	{
	public:
		float positionX = 0.f;
		float positionY = 0.f;
		float angle = 0.f;

		virtual ~Mesh();
		virtual void draw();

		static std::vector< Mesh* > meshes;
	};


	std::vector< Mesh* > Mesh::meshes;


	Mesh::~Mesh()
	{
	}


	void Mesh::draw()
	{
		glLoadIdentity();
		glTranslatef( positionX, positionY, 0.f );
		glRotatef( angle * 180.f / pi, 0.f, 0.f, 1.f );
	}


	template< class MeshClass, class... Args >
	Mesh* createMesh( Args&&... args )
	{
		Mesh* mesh = new MeshClass( std::forward< Args >( args )... );
		Mesh::meshes.push_back( mesh );
		return mesh;
	}


	void destroyMesh( Mesh* mesh )
	{
		auto it = std::find( Mesh::meshes.begin(), Mesh::meshes.end(), mesh );
		assert( it != Mesh::meshes.end() );
		Mesh::meshes.erase( it );
		delete mesh;
	}


	void placeMesh( Mesh* mesh, float x, float y, float angle )
	{
		mesh->positionX = x;
		mesh->positionY = y;
		mesh->angle = angle;
	}
}


//-------------------------------------------------------
//	user interface: ball mesh support
//-------------------------------------------------------

namespace Scene
{
	namespace
	{
		class CircleMesh : public Mesh
		{
		public:
			CircleMesh( float radius, Color color );
			void draw() override;

		private:
			float const radius;
			Color const color;
		};


		CircleMesh::CircleMesh( float radius, Color color ) :
			radius( radius ),
			color( color )
		{
		}


		void CircleMesh::draw()
		{
			Mesh::draw();

			constexpr int numTriangles = 16;

			glBegin( GL_TRIANGLES );
			setupGLColor( color );
			for ( int i = 0; i < numTriangles; i++ )
			{
				float angle1 = float( i ) / float( numTriangles ) * 2.f * pi;
				float angle2 = float( i + 1 ) / float( numTriangles ) * 2.f * pi;
				glVertex2f( radius * std::cos( angle1 ), radius * std::sin( angle1 ) );
				glVertex2f( 0.f, 0.f );
				glVertex2f( radius * std::cos( angle2 ), radius * std::sin( angle2 ) );
			}
			glEnd();
		}
	}


	Mesh *createBallMesh( float radius )
	{
		return createMesh< CircleMesh >( radius, Color::white );
	}


	Mesh *createPocketMesh( float radius )
	{
		return createMesh< CircleMesh >( radius, Color::red );
	}
}


//-------------------------------------------------------
// user interface: frame support
//-------------------------------------------------------

namespace Scene
{
	namespace
	{
		namespace Background
		{
			float width = 0.f;
			float height = 0.f;


			void draw()
			{
				auto drawRectangle = []( float left, float top, float right, float bottom ) -> void
				{
					glColor3f( 0.05f, 0.05f, 0.05f );
					glBegin( GL_TRIANGLE_STRIP );
					glVertex2f( left, top );
					glVertex2f( right, top );
					glVertex2f( left, bottom );
					glVertex2f( right, bottom );
					glEnd();
				};

				constexpr float viewHalfWidth = 0.5f * View::width;
				constexpr float viewHalfHeight = 0.5f * View::height;
				const float backHalfWidth = 0.5f * Background::width;
				const float backHalfHeight = 0.5f * Background::height;

				glLoadIdentity();
				drawRectangle( -viewHalfWidth, viewHalfHeight, -backHalfWidth, -viewHalfHeight );
				drawRectangle( backHalfWidth, viewHalfHeight, viewHalfWidth, -viewHalfHeight );
				drawRectangle( -backHalfWidth, viewHalfHeight,backHalfWidth, backHalfHeight );
				drawRectangle( -backHalfWidth, -backHalfHeight,backHalfWidth, -viewHalfHeight );
			}
		}
	}


	void setupBackground( float width, float height )
	{
		Background::width = width;
		Background::height = height;
	}
}


//-------------------------------------------------------
// user interface: progress bar support
//-------------------------------------------------------

namespace Scene
{
	namespace
	{
		namespace ProgressBar
		{
			float value = 0.f;

			float left = -3.f;
			float right = 3.f;
			float top = -4.f;
			float bottom = -4.5f;


			void draw()
			{
				glLoadIdentity();
				glColor3f( 1.f, 0.f, 1.f );
				glBegin( GL_TRIANGLE_STRIP );
				glVertex2f( left, top );
				glVertex2f( left + value * ( right - left ), top );
				glVertex2f( left, bottom );
				glVertex2f( left + value * ( right - left ), bottom );
				glEnd();
			}
		}
	}


	void updateProgressBar( float progress )
	{
		ProgressBar::value = std::max( std::min( progress, 1.f ), 0.f );
	}
}


//-------------------------------------------------------
//	engine only interface
//-------------------------------------------------------

namespace Scene
{
	void draw()
	{
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glScalef( 2.f / View::width, 2.f / View::height, 0.f );

		glDisable( GL_CULL_FACE );
		glClearColor( 0.1f, 0.4f, 0.2f, 0.f );
		glClear( GL_COLOR_BUFFER_BIT );
		glMatrixMode( GL_MODELVIEW );

		for ( Mesh *mesh : Mesh::meshes )
			mesh->draw();

		Background::draw();
		ProgressBar::draw();
	}


	float screenToWorldX( float x )
	{
		return 0.5f * View::width * ( 2.f * x - 1.f );
	}


	float screenToWorldY( float y )
	{
		return 0.5f * View::height * ( 2.f * y - 1.f );
	}
}
