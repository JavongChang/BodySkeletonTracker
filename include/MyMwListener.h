/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/

#ifndef MYMWLISTENER_H
#define MYMWLISTENER_H

#include <OpenNI.h>

class MyMwListener : public closest_point::ClosestPoint::Listener
{
public:
	MyMwListener() : m_ready(false) {}
	virtual ~MyMwListener() {}
	void readyForNextData(closest_point::ClosestPoint* pClosestPoint)
	{
		openni::Status rc = pClosestPoint->getNextData(m_closest, m_frame);

		if (rc == openni::STATUS_OK)
		{
			//printf("pontos::%d, %d, %d\n", m_closest.X, m_closest.Y, m_closest.Z);
		}
		else
		{
			printf("Update failed\n");
		}
		m_ready = true;
	}


	const openni::VideoFrameRef& getFrame() {return m_frame;}
	const closest_point::IntPoint3D& getClosestPoint() {return m_closest;}
	bool isAvailable() const {return m_ready;}
	void setUnavailable() {m_ready = false;}
private:
	openni::VideoFrameRef m_frame;
	closest_point::IntPoint3D m_closest;
	bool m_ready;
};

#endif
