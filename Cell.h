#pragma once
#include <cstdint>


struct Edge
{
	float sx, sy;
	float ex, ey;
};

class Cell
{
public:
	int16_t edgeId[4];
	bool edgeAt[4];
	bool edgeExist;

public:
	Cell()
	{
		edgeExist = false;
	}

	int16_t* getEdgeId();
	bool* getEdgeAt();
	bool getEdgeExist();

};