/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor: 
 *		Jeroen Bakker 
 *		Monique Dewanchand
 */

#include "COM_BilateralBlurOperation.h"
#include "BLI_math.h"

extern "C" {
	#include "RE_pipeline.h"
}

BilateralBlurOperation::BilateralBlurOperation() : NodeOperation()
{
	this->addInputSocket(COM_DT_COLOR);
	this->addInputSocket(COM_DT_COLOR);
	this->addOutputSocket(COM_DT_COLOR);
	this->setComplex(true);

	this->inputColorProgram = NULL;
	this->inputDeterminatorProgram = NULL;
}

void BilateralBlurOperation::initExecution()
{
	this->inputColorProgram = getInputSocketReader(0);
	this->inputDeterminatorProgram = getInputSocketReader(1);
	this->space = this->data->sigma_space + this->data->iter;
	QualityStepHelper::initExecution(COM_QH_INCREASE);
}

void BilateralBlurOperation::executePixel(float *color, int x, int y, MemoryBuffer *inputBuffers[], void *data)
{
	// read the determinator color at x, y, this will be used as the reference color for the determinator
	float determinatorReferenceColor[4];
	float determinator[4];
	float tempColor[4];
	float blurColor[4];
	float blurDivider;
	float space = this->space;
	float sigmacolor = this->data->sigma_color;
	int minx = floor(x - space);
	int maxx = ceil(x + space);
	int miny = floor(y - space);
	int maxy = ceil(y + space);
	float deltaColor;
	this->inputDeterminatorProgram->read(determinatorReferenceColor, x, y, inputBuffers, data);

	zero_v4(blurColor);
	blurDivider = 0.0f;
	for (int yi = miny ; yi < maxy ; yi+=QualityStepHelper::getStep()) {
		for (int xi = minx ; xi < maxx ; xi+=QualityStepHelper::getStep()) {
			// read determinator
			this->inputDeterminatorProgram->read(determinator, xi, yi, inputBuffers, data);
			deltaColor = (fabsf(determinatorReferenceColor[0] - determinator[0]) +
			              fabsf(determinatorReferenceColor[1] - determinator[1]) +
			              fabsf(determinatorReferenceColor[2] - determinator[2])); // do not take the alpha channel into account
			if (deltaColor< sigmacolor) {
				// add this to the blur
				this->inputColorProgram->read(tempColor, xi, yi, inputBuffers, data);
				add_v4_v4(blurColor, tempColor);
				blurDivider += 1.0f;
			}
		}
	}
	
	if (blurDivider > 0.0f) {
		mul_v4_v4fl(color, blurColor, 1.0f / blurDivider);
	}
	else {
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 1.0f;
	}
}

void BilateralBlurOperation::deinitExecution()
{
	this->inputColorProgram = NULL;
	this->inputDeterminatorProgram = NULL;
}

bool BilateralBlurOperation::determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output)
{
	rcti newInput;
	int add = ceil(this->space)+1;

	newInput.xmax = input->xmax + (add);
	newInput.xmin = input->xmin - (add);
	newInput.ymax = input->ymax + (add);
	newInput.ymin = input->ymin - (add);

	return NodeOperation::determineDependingAreaOfInterest(&newInput, readOperation, output);
}
