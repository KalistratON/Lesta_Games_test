
#pragma once


//-------------------------------------------------------
//	user interface
//-------------------------------------------------------

namespace Scene
{
	class Mesh;

	Mesh* createBallMesh( float radius );
	Mesh* createPocketMesh( float radius );
	void destroyMesh( Mesh* mesh );
	void placeMesh( Mesh* mesh, float x, float y, float angle );

	void setupBackground( float width, float height );

	void updateProgressBar( float progress );
}


//-------------------------------------------------------
//	engine only interface
//-------------------------------------------------------

namespace Scene
{
	void draw();
	float screenToWorldX( float x );
	float screenToWorldY( float x );
}
