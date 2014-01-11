// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "kernels/xeonphi/bvh4mb/bvh4mb.h"
#include "kernels/xeonphi/bvh4mb/bvh4mb_builder.h"
#include "geometry/triangle1.h"
#include "common/accelinstance.h"

namespace embree
{

  DECLARE_SYMBOL(Accel::Intersector1,BVH4mbTriangle1Intersector1);
  DECLARE_SYMBOL(Accel::Intersector16,BVH4mbTriangle1Intersector16ChunkMoeller);

  void BVH4mbRegister () 
  {
    int features = getCPUFeatures();

    /* default target */
    SELECT_SYMBOL_KNC(features,BVH4mbTriangle1Intersector1);
    SELECT_SYMBOL_KNC(features,BVH4mbTriangle1Intersector16ChunkMoeller);
  }

  Accel::Intersectors BVH4mbTriangle1Intersectors(BVH4mb* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1  = BVH4mbTriangle1Intersector1;
    intersectors.intersector16 = BVH4mbTriangle1Intersector16ChunkMoeller;
    return intersectors;
  }

  Accel* BVH4mb::BVH4mbTriangle1ObjectSplitBinnedSAH(Scene* scene)
  { 
    BVH4mb* accel = new BVH4mb(SceneTriangle1::type);   
    Builder* builder = BVH4mbBuilder::create(accel,&scene->flat_triangle_source_1,scene);    
    Accel::Intersectors intersectors = BVH4mbTriangle1Intersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  BVH4mb::~BVH4mb()
  {
    if (qbvh)  os_free(qbvh,size_node);
    if (accel) os_free(accel,size_accel);
    if (accel_t1) os_free(accel_t1,size_accel);
  }


}
