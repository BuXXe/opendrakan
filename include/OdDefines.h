/*
 * OdDefines.h
 *
 *  Created on: 31 Jan 2018
 *      Author: zal
 */

#ifndef INCLUDE_ODDEFINES_H_
#define INCLUDE_ODDEFINES_H_

#define OD_LIB_NAME "od"

#define OD_WORLD_SCALE (1.0/(1 << 11))

// TODO: put this somewhere where it belongs
#define OD_SHADER_DEFAULT_VERTEX   "default_vertex.glsl"
#define OD_SHADER_DEFAULT_FRAGMENT "default_fragment.glsl"
#define OD_SHADER_RIGGED_VERTEX    "rigged_vertex.glsl"

#define OD_MAX_BONE_COUNT 64
#define OD_ATTRIB_INFLUENCE_LOCATION 4
#define OD_ATTRIB_WEIGHT_LOCATION 5

#endif /* INCLUDE_ODDEFINES_H_ */
