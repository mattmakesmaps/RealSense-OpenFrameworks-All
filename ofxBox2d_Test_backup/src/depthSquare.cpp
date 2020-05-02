#include "depthSquare.h"

DepthSquare::DepthSquare(const int& x, const int& y, const int& length)
{
	m_x = x;
	m_y = y;
	m_length = length;
	m_depth = 0;
	m_color = 0;
}

void DepthSquare::draw()
{
	auto lerped_depth = ofLerp(0, 255, m_depth);
	ofSetColor(lerped_depth);
	ofDrawRectangle(m_x, m_y, m_length, m_length);
}

void DepthSquare::setDepth(const float& depth)
{
	m_depth = depth;
}
