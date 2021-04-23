#include <iostream>
#include <vector>
#include <tuple>

//this is meant to be here
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "Cell.h"

const int8_t NORTH = 0;
const int8_t SOUTH = 1;
const int8_t EAST = 2;
const int8_t WEST = 3;

class ShadowCasting : public olc::PixelGameEngine
{
public:
	ShadowCasting()
	{
		sAppName = "ShadowCasting2D";
	}

public:
	bool OnUserCreate() override
	{
		world.resize(worldHeigth * worldWidth);

		// Add a boundary to the world
		for (int x = 1; x < (worldWidth - 1); x++)
		{
			world[1 * worldWidth + x].edgeExist = true;
			world[(worldHeigth - 2) * worldWidth + x].edgeExist = true;
		}

		for (int x = 1; x < (worldHeigth - 1); x++)
		{
			world[x * worldWidth + 1].edgeExist = true;
			world[x * worldWidth + (worldWidth - 2)].edgeExist = true;
		}

		sprite = new olc::Sprite("light-ray.png");
		lightRay = new olc::Sprite(ScreenWidth(), ScreenHeight());
		lightTex = new olc::Sprite(ScreenWidth(), ScreenHeight());

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		float fBlockWidth = 16.0f;
		float fMouseX = GetMouseX();
		float fMouseY = GetMouseY();

		if (GetMouse(2).bHeld)
		{
			// i = y * width + x
			int i = ((int)fMouseY / (int)fBlockWidth) * this->worldWidth + ((int)fMouseX / (int)fBlockWidth);
			world[i].edgeExist = true;// !world[i].getEdgeExist();
		}

		if (GetMouse(1).bHeld)
		{
			// i = y * width + x
			int i = ((int)fMouseY / (int)fBlockWidth) * this->worldWidth + ((int)fMouseX / (int)fBlockWidth);
			world[i].edgeExist = false;// !world[i].getEdgeExist();
		}

		TileToPoly(0, 0, worldWidth, worldHeigth, fBlockWidth, worldWidth);

		if (GetMouse(0).bHeld)
		{
			CalculatePolygoneVisibility(fMouseX, fMouseY, 1000.0f);
		}





		//Draw
		//SetDrawTarget(nullptr);
		Clear(olc::BLACK);

		auto it = unique(
			visibilityPoints.begin(),
			visibilityPoints.end(),
			[&](const std::tuple<float, float, float>& obj1, const std::tuple<float, float, float>& obj2)
			{
				return fabs(std::get<1>(obj1) - std::get<1>(obj2)) < 0.01f && fabs(std::get<2>(obj1) - std::get<2>(obj2)) < 0.01f;
			});

		visibilityPoints.resize(std::distance(visibilityPoints.begin(), it));

		if (GetMouse(0).bHeld && visibilityPoints.size() > 1)
		{

			SetDrawTarget(lightTex);
			Clear(olc::BLACK);

			DrawSprite(fMouseX - 240, fMouseY - 240, sprite);


			SetDrawTarget(lightRay);
			Clear(olc::BLANK);

			//draw rays
			for (int index = 0; index < visibilityPoints.size() - 1; ++index)
			{
				FillTriangle(
					fMouseX,
					fMouseY,
					std::get<1>(visibilityPoints[index]),
					std::get<2>(visibilityPoints[index]),
					std::get<1>(visibilityPoints[index + 1]),
					std::get<2>(visibilityPoints[index + 1]),
					olc::DARK_YELLOW
				);
			}

			//close the last ray
			FillTriangle(
				fMouseX,
				fMouseY,
				std::get<1>(visibilityPoints[0]),
				std::get<2>(visibilityPoints[0]),
				std::get<1>(visibilityPoints[visibilityPoints.size() - 1]),
				std::get<2>(visibilityPoints[visibilityPoints.size() - 1]),
				olc::DARK_YELLOW
			);

			SetDrawTarget(nullptr);

			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
					if (lightRay->GetPixel(x, y).r > 0)
						Draw(x, y, lightTex->GetPixel(x, y));
		}

		for (int16_t xIndex = 0; xIndex < worldWidth; ++xIndex)
			for (int16_t yIndex = 0; yIndex < worldHeigth; ++yIndex)
			{
				if (world[yIndex * worldWidth + xIndex].edgeExist)
				{
					FillRect(xIndex * fBlockWidth, yIndex * fBlockWidth, fBlockWidth, fBlockWidth, olc::GREEN);
				}
			}

		for (auto &edge : edges)
		{
			DrawLine(edge.sx, edge.sy, edge.ex, edge.ey);
			FillCircle(edge.sx, edge.sy, 3, olc::RED);
			FillCircle(edge.ex, edge.ey, 3, olc::RED);
		}

		return true;
	}

private:

	void TileToPoly(int sx, int sy, int width, int heigth, float fBlockWidth, int pitch)
	{
		edges.clear();

		for (int16_t xIndex = 0; xIndex < width; ++xIndex)
			for (int16_t yIndex = 0; yIndex < heigth; ++yIndex)
				for (int8_t location = 0; location < 4; ++location)
				{
					world[(yIndex + sy) * pitch + (xIndex + sx)].edgeAt[location] = false;
					world[(yIndex + sy) * pitch + (xIndex + sx)].edgeId[location] = 0;
				}

		for (int16_t xIndex = 1; xIndex < width - 1; ++xIndex)
			for (int16_t yIndex = 1; yIndex < heigth - 1; ++yIndex)
			{
				int myExactLocation = (yIndex + sy) * pitch + (xIndex + sx);
				int north = (yIndex + sy - 1) * pitch + (xIndex + sx);
				int south = (yIndex + sy + 1) * pitch + (xIndex + sx);
				int east = (yIndex + sy) * pitch + (xIndex + sx + 1);
				int west = (yIndex + sy) * pitch + (xIndex + sx - 1);

				if (world[myExactLocation].edgeExist)
				{
					if (!world[west].edgeExist)
					{
						if (world[north].edgeAt[WEST])
						{
							edges[world[north].edgeId[WEST]].ey += fBlockWidth;
							world[myExactLocation].edgeId[WEST] = world[north].edgeId[WEST];
							world[myExactLocation].edgeAt[WEST] = true;
						}
						else
						{
							Edge edge;
							edge.sx = (sx + xIndex) * fBlockWidth;
							edge.sy = (sy + yIndex) * fBlockWidth;

							edge.ex = edge.sx;
							edge.ey = edge.sy + fBlockWidth;

							int16_t idOfEdge = edges.size();
							edges.push_back(edge);

							world[myExactLocation].edgeId[WEST] = idOfEdge;
							world[myExactLocation].edgeAt[WEST] = true;
						}
					}

					if (!world[east].edgeExist)
					{
						if (world[north].edgeAt[EAST])
						{
							edges[world[north].edgeId[EAST]].ey += fBlockWidth;
							world[myExactLocation].edgeId[EAST] = world[north].edgeId[EAST];
							world[myExactLocation].edgeAt[EAST] = true;
						}
						else
						{
							Edge edge;
							edge.sx = (sx + xIndex + 1) * fBlockWidth;
							edge.sy = (sy + yIndex) * fBlockWidth;

							edge.ex = edge.sx;
							edge.ey = edge.sy + fBlockWidth;

							int16_t idOfEdge = edges.size();
							edges.push_back(edge);

							world[myExactLocation].edgeId[EAST] = idOfEdge;
							world[myExactLocation].edgeAt[EAST] = true;
						}
					}

					if (!world[north].edgeExist)
					{
						if (world[west].edgeAt[NORTH])
						{
							edges[world[west].edgeId[NORTH]].ex += fBlockWidth;
							world[myExactLocation].edgeId[NORTH] = world[west].edgeId[NORTH];
							world[myExactLocation].edgeAt[NORTH] = true;
						}
						else
						{
							Edge edge;
							edge.sx = (sx + xIndex) * fBlockWidth;
							edge.sy = (sy + yIndex) * fBlockWidth;

							edge.ex = edge.sx + fBlockWidth;
							edge.ey = edge.sy;

							int16_t idOfEdge = edges.size();
							edges.push_back(edge);

							world[myExactLocation].edgeId[NORTH] = idOfEdge;
							world[myExactLocation].edgeAt[NORTH] = true;
						}
					}

					if (!world[south].edgeExist)
					{
						if (world[west].edgeAt[SOUTH])
						{
							edges[world[west].edgeId[SOUTH]].ex += fBlockWidth;
							world[myExactLocation].edgeId[SOUTH] = world[west].edgeId[SOUTH];
							world[myExactLocation].edgeAt[SOUTH] = true;
						}
						else
						{
							Edge edge;
							edge.sx = (sx + xIndex) * fBlockWidth;
							edge.sy = (sy + yIndex + 1) * fBlockWidth;

							edge.ex = edge.sx + fBlockWidth;
							edge.ey = edge.sy;

							int16_t idOfEdge = edges.size();
							edges.push_back(edge);

							world[myExactLocation].edgeId[SOUTH] = idOfEdge;
							world[myExactLocation].edgeAt[SOUTH] = true;
						}
					}
				}

			}

	}
	void CalculatePolygoneVisibility(float ox, float oy, float radius)
	{
		visibilityPoints.clear();

		for (auto& itr : edges)
		{
			for (int index = 0; index < 2; ++index)
			{
				float radiusX, radiusY;

				//gradients
				radiusX = ((index == 0) ? itr.sx : itr.ex) - ox;
				radiusY = ((index == 0) ? itr.sy : itr.ey) - oy;

				//basic angle
				float baseAngle = atan2f(radiusY, radiusX);

				float angle = 0;

				for (int deviation = 0; deviation < 3; ++deviation)
				{
					//small deviation
					//3 angles;

					switch (deviation)
					{
					case 0:
						angle = baseAngle - 0.0001f;
						break;
					case 1:
						angle = baseAngle;
						break;
					case 2:
						angle = baseAngle + 0.0001f;
						break;
					}

					radiusX = radius * cosf(angle);
					radiusY = radius * sinf(angle);

					float minT1 = INFINITY;
					float minPx = 0;
					float minPy = 0;
					float minAngle = 0;
					bool interValid = false;

					for (auto& currInter : edges)
					{
						//check for intersections

						float vectorX = currInter.ex - currInter.sx;
						float vectorY = currInter.ey - currInter.sy;


						//check if ther are colinear
						if (fabs(vectorX - radiusX) > 0.0f && fabs(vectorY - radiusY) > 0.0f)
						{
							// t2 is normalised distance from line segment start to line segment end of intersect point
							float t2 = (radiusX * (currInter.sy - oy) + radiusY * (ox - currInter.sx)) / (vectorX * radiusY - vectorY * radiusX);
							// t1 is normalised distance from source along ray to ray length of intersect point
							float t1 = (currInter.sx + vectorX * t2 - ox) / radiusX;

							if (t1 > 0 && t2 >= 0 && t2 <= 1.0f)
							{
								if (t1 < minT1)
								{
									minT1 = t1;

									//calculate new intersection point 
									minPx = ox + radiusX * t1;
									minPy = oy + radiusY * t1;
									minAngle = atan2f(minPy - oy, minPx - ox);
									interValid = true;
								}
							}
						}
					}

					if (interValid)
					{
						visibilityPoints.push_back({ minAngle, minPx, minPy });
					}

				}
			}
		}

		std::sort
		(
			visibilityPoints.begin(),
			visibilityPoints.end(),
			//custom comparator just for the angles
			[&](const std::tuple<float, float, float>& obj1, const std::tuple<float, float, float>& obj2)
			{
				return std::get<0>(obj1) < std::get<0>(obj2);
			}
		);
	}

private:
	std::vector<Cell> world;
	int16_t worldWidth = 40;
	int16_t worldHeigth = 30;

	olc::Sprite* sprite;
	olc::Sprite* lightRay;
	olc::Sprite* lightTex;

	std::vector<Edge> edges;

	std::vector<std::tuple<float, float, float>> visibilityPoints;

};


int main()
{
	ShadowCasting mainWindow;

	if (mainWindow.Construct(640, 480, 2, 2))
	{
		mainWindow.Start();
	}
	else
	{
		//empty
	}
}
