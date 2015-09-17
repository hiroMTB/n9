#pragma once
#include "cinder/CinderResources.h"

#define RES_PASSTHRU_VERT	CINDER_RESOURCE( ../shader/, passThru_vert.glsl, 128, GLSL )
#define RES_BLUR_FRAG		CINDER_RESOURCE( ../shader/, gaussianBlur_frag.glsl, 129, GLSL )
#define RES_IMAGE_JPG		CINDER_RESOURCE( ../shader/, cinder_logo.png, 130, IMAGE )
