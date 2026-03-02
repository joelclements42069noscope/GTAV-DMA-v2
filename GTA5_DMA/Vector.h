#pragma once

struct Vec3
{
	float x, y, z;

public:
	float Distance(Vec3 Destination)
	{
		float dx = (Destination.x - this->x);
		float dy = (Destination.y - this->y);
		float dz = (Destination.z - this->z);
		return sqrt(dx * dx + dy * dy + dz * dz);
	}

	float Distance2D(Vec3 Destination)
	{
		float dx = (Destination.x - this->x);
		float dy = (Destination.y - this->y);
		return sqrt(dx * dx + dy * dy);
	}
};