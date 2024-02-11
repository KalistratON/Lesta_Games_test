
#include <cassert>
#include <cmath>
#include <array>
#include <unordered_set>
#include <iostream>

#include "../framework/scene.hpp"
#include "../framework/game.hpp"
#include "../framework/engine.hpp"


//-------------------------------------------------------
//	Basic Vector2 class
//-------------------------------------------------------

class Vector2
{
public:
	float x = 0.f;
	float y = 0.f;

	constexpr Vector2() : x(.0), y(.0) {};
	constexpr Vector2( float vx, float vy );
	constexpr Vector2( Vector2 const &other ) = default;

	Vector2  operator -  (const Vector2& another) const;
	Vector2  operator +  (const Vector2& another) const;
	float    operator *  (const Vector2& another) const;
	Vector2  operator *  (float scale) const;
	Vector2& operator -= (float scale);
	Vector2& operator *= (float scale);
	
	const Vector2& normolize();

	float   norm()		const;
	float   length()	const;
};


constexpr Vector2::Vector2( float vx, float vy ) :
	x( vx ),
	y( vy )
{
}

Vector2 Vector2::operator - (const Vector2& another) const
{
	return Vector2{ x - another.x, y - another.y };
}

Vector2 Vector2::operator + (const Vector2& another) const
{
	return Vector2{ x + another.x, y + another.y };
}

float Vector2::operator * (const Vector2& another) const
{
	return x * another.x + y * another.y;
}

Vector2& Vector2::operator -= (float scale)
{
	this->x -= scale;
	this->y -= scale;
	return *this;
}

Vector2& Vector2::operator *= (float scale)
{
	this->x *= scale;
	this->y *= scale;
	return *this;
}

Vector2 Vector2::operator * (float scale) const
{
	return Vector2{ x * scale, y * scale };
}

Vector2 operator * (float scale, const Vector2& vec)
{
	return vec * scale;
}

const Vector2& Vector2::normolize()
{
	float len = length();
	x /= len;
	y /= len;
	return *this;
}

float Vector2::norm() const
{
	return x * x + y * y;
}

float Vector2::length() const
{
	return sqrt(norm());
}

//-------------------------------------------------------
//	game parameters
//-------------------------------------------------------

namespace Params
{
	namespace System
	{
		constexpr int targetFPS = 60;
		constexpr float accurance = 0.01f;
	}

	namespace Table
	{
		constexpr float width = 15.f;
		constexpr float height = 8.f;
		constexpr float pocketRadius = 0.4f;

		static constexpr std::array< Vector2, 6 > pocketsPositions =
		{
			Vector2{ -0.5f * width, -0.5f * height },
			Vector2{ 0.f, -0.5f * height },
			Vector2{ 0.5f * width, -0.5f * height },
			Vector2{ -0.5f * width, 0.5f * height },
			Vector2{ 0.f, 0.5f * height },
			Vector2{ 0.5f * width, 0.5f * height }
		};

		static constexpr std::array< Vector2, 7 > ballsPositions =
		{
			// player ball
			Vector2( -0.3f * width, 0.f ),
			// other balls
			Vector2( 0.2f * width, 0.f ),
			Vector2( 0.25f * width, 0.05f * height ),
			Vector2( 0.25f * width, -0.05f * height ),
			Vector2( 0.3f * width, 0.1f * height ),
			Vector2( 0.3f * width, 0.f ),
			Vector2( 0.3f * width, -0.1f * height )
		};
	}

	namespace Ball
	{
		constexpr float radius = 0.3f;
	}

	namespace Shot
	{
		constexpr float chargeTime = 1.f;
	}
}


//-------------------------------------------------------
//	Table logic
//-------------------------------------------------------

class Table
{
public:
	Table() = default;
	Table( Table const& ) = delete;

	void init();
	void deinit();

	const std::array< Scene::Mesh*, 7 >& getBalls() const;

private:
	std::array< Scene::Mesh*, 6 > pockets = {};
	std::array< Scene::Mesh*, 7 > balls = {};
};


void Table::init()
{
	for ( int i = 0; i < 6; i++ )
	{
		assert( !pockets[ i ] );
		pockets[ i ] = Scene::createPocketMesh( Params::Table::pocketRadius );
		Scene::placeMesh( pockets[ i ], Params::Table::pocketsPositions[ i ].x, Params::Table::pocketsPositions[ i ].y, 0.f );
	}

	for ( int i = 0; i < 7; i++ )
	{
		assert( !balls[ i ] );
		balls[ i ] = Scene::createBallMesh( Params::Ball::radius );
		Scene::placeMesh( balls[ i ], Params::Table::ballsPositions[ i ].x, Params::Table::ballsPositions[ i ].y, 0.f );
	}
}


void Table::deinit()
{
	for ( Scene::Mesh* mesh : pockets )
		Scene::destroyMesh( mesh );
	for ( Scene::Mesh* mesh : balls )
		Scene::destroyMesh( mesh );

	pockets = {};
	balls = {};
}

const std::array< Scene::Mesh*, 7 >& Table::getBalls() const
{
	return balls;
}


//-------------------------------------------------------
//	game public interface
//-------------------------------------------------------

namespace Game
{
	Table table;

	bool isChargingShot = false;
	float shotChargeProgress = 0.f;

	const float infinity = 2 * (Params::Table::height + Params::Table::width);
	const float impulse  = 6.0f;
	const float friction = 0.03f;

	std::array< Vector2, 7 > ballPositions = {};
	std::array< Vector2, 7 > ballVelocities = {};

	void init()
	{
		Engine::setTargetFPS( Params::System::targetFPS );
		Scene::setupBackground( Params::Table::width, Params::Table::height );
		table.init();

		ballPositions  = Params::Table::ballsPositions;
		ballVelocities = {};
	}

	void deinit()
	{
		table.deinit();
	}

	size_t findClosestBall(const Vector2& ballEndPosition, size_t subject)
	{
		size_t index = subject;
		float distance = infinity;

		for (size_t i = 0; i < ballPositions.size(); ++i) {
			if (i == subject) {
				continue;
			}
			if ((ballEndPosition - ballPositions[i]).length() < 2 * Params::Ball::radius)
			{
				float curr_distance = (ballPositions[subject] - ballPositions[i]).length();
				if (curr_distance < distance) {
					distance = curr_distance;
					index = i;
				}
			}
		}
		return index;
	}

	void recalculateVelocities(size_t subject, size_t target)
	{
		const auto& dir = (ballPositions[target] - ballPositions[subject]).normolize();
		Vector2 tan = { dir.y, dir.x };
		
		auto dirSubjectVel = ballVelocities[subject] * dir;
		auto dirTargetVel = ballVelocities[target] * dir;

		auto tanSubjectVel = ballVelocities[subject] - dir * dirSubjectVel;
		auto tanTargetVel = ballVelocities[target] - dir * dirTargetVel;

		ballVelocities[subject] = tanSubjectVel + dir * dirTargetVel;
		ballVelocities[target] = tanTargetVel + dir * dirSubjectVel;
	}

	float calculateTimeConflict(float distance, float velocityModule)
	{
		return (distance - 2 * Params::Ball::radius) / velocityModule;
	}

	bool inPocket(const Vector2& ballEndPos)
	{
		for (const auto& pocketPos : Params::Table::pocketsPositions) {
			if ((ballEndPos - pocketPos).length() <= Params::Table::pocketRadius + Params::Ball::radius / 4.f) {
				return true;
			}
		}
		return false;
	}

	void reduceVelocities(float dt)
	{
		for (auto& velocity : ballVelocities) {
			
			if (velocity.length() < Params::System::accurance) {
				continue;
			}

			bool isVxPositive = velocity.x >= 0.f;
			bool isVyPositive = velocity.y >= 0.f;
			
			velocity.x -= isVxPositive ? friction * dt * 9.81f : -friction * dt * 9.81f;
			velocity.y -= isVyPositive ? friction * dt * 9.81f : -friction * dt * 9.81f;

			if (velocity.x < 0.f && isVxPositive ||
				velocity.x > 0.f && !isVxPositive) {
				velocity.x = 0.f;
			}
			if (velocity.y < 0.f && isVyPositive ||
				velocity.y > 0.f && !isVyPositive) {
				velocity.y = 0.f;
			}
		}
	}

	bool isBorderCollesion(const Vector2& ballPos, size_t i)
	{
		bool borderCollesion = false;
		if (abs(abs(ballPos.x) - Params::Table::width / 2.f) <= Params::Ball::radius + Params::System::accurance) {
			ballVelocities[i].x = -ballVelocities[i].x;
			borderCollesion = true;
		}
		if (abs(abs(ballPos.y) - Params::Table::height / 2.f) <= Params::Ball::radius + Params::System::accurance) {
			ballVelocities[i].y = -ballVelocities[i].y;
			borderCollesion = true;
		}
		return borderCollesion;
	}
	
	bool isFreeze()
	{
		for (const auto& velocities : ballVelocities) {
			if (velocities.length() < Params::System::accurance) {
				return false;
			}
		}
		return true;
	}

	void physicLoop(float dt)
	{
		if (isFreeze()) {
			return;
		}

		const auto& balls = table.getBalls();
		std::unordered_set<size_t> undoneBalls;
		for (size_t i = 0; i < ballVelocities.size(); ++i) {
			
			if (ballVelocities[i].length() <= Params::System::accurance ||
				undoneBalls.find(i) != undoneBalls.end()) {
				continue;
			}

			Vector2 ballEndPos = {
							ballPositions[i].x + ballVelocities[i].x * dt,
							ballPositions[i].y + ballVelocities[i].y * dt };
			
			if (inPocket(ballEndPos)) {
				if (i == 0) {
					deinit();
					init();

					return;
				}

				ballPositions[i] = { infinity, infinity };
				ballVelocities[i] = { 0.f, 0.f };
				Scene::placeMesh(balls[i], ballPositions[i].x, ballPositions[i].y, 0.f);

				continue;
			}
						
			if (isBorderCollesion(ballEndPos, i)) {
				continue;
			}

			size_t j = findClosestBall(ballEndPos, i);
			if (j == i) {
				ballPositions[i] = ballEndPos;
				Scene::placeMesh(balls[i], ballPositions[i].x, ballPositions[i].y, 0.f);

				continue;
			}

			// calc approximated [dt << 1] subject position when conflict
			float distance = (ballPositions[i] - ballPositions[j]).length();
			float dtau = calculateTimeConflict(distance, ballVelocities[i].length());
			Vector2 conflictSubjectPos = { ballPositions[i].x + ballVelocities[i].x * dtau,
										   ballPositions[i].y + ballVelocities[i].y * dtau };
			
			// calc new position
			recalculateVelocities(i, j);
			ballPositions[i] = { conflictSubjectPos.x + ballVelocities[i].x * (dt - dtau),
								 conflictSubjectPos.y + ballVelocities[i].y * (dt - dtau) };
			ballPositions[j] = { ballPositions[j].x + ballVelocities[j].x * (dt - dtau),
								 ballPositions[j].y + ballVelocities[j].y * (dt - dtau) };

			Scene::placeMesh(balls[i], ballPositions[i].x, ballPositions[i].y, 0.f);
			Scene::placeMesh(balls[j], ballPositions[j].x, ballPositions[j].y, 0.f);

			undoneBalls.insert({ i, j });
		}
		reduceVelocities(dt);
	}

	void update( float dt )
	{
		physicLoop(dt);
		if ( isChargingShot )
			shotChargeProgress = std::min( shotChargeProgress + dt / Params::Shot::chargeTime, 1.f );
		Scene::updateProgressBar( shotChargeProgress );
	}

	void mouseButtonPressed( float x, float y )
	{
		isChargingShot = true;
	}

	void mouseButtonReleased( float x, float y )
	{
		// TODO: implement billiard logic here
		 
		// New shot can't be done while all balls are in motion;
		if (ballVelocities[0].length() < 0.01f) {
			ballVelocities[0] = { x - ballPositions[0].x, 
								  y - ballPositions[0].y };
			ballVelocities[0].normolize();
			ballVelocities[0] *= impulse * shotChargeProgress;
		}

		isChargingShot = false;
		shotChargeProgress = 0.f;
	}
}
