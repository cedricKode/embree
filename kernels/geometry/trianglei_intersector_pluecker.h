// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#pragma once

#include "trianglei.h"
#include "trianglei_mb.h"
#include "../common/ray.h"

#include "triangle_intersector_pluecker.h"
#include "../common/scene_triangle_mesh.h"

namespace embree
{
  namespace isa
  {
    /*! Intersects M triangles with 1 ray */
    template<int M, int Mx, bool filter>
    struct TriangleMiIntersector1Pluecker
    {
      typedef TriangleMi<M> Primitive;
      typedef PlueckerIntersector1<Mx> Precalculations;
      
      static __forceinline void intersect(const Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive& tri, Scene* scene, const unsigned* geomID_to_instID)
      {
        STAT3(normal.trav_prims,1,1,1);
        Vec3<vfloat<M>> v0, v1, v2; tri.gather(v0,v1,v2);
        pre.intersect(ray,v0,v1,v2,UVIdentity<Mx>(),Intersect1EpilogM<M,Mx,filter>(ray,context,tri.geomIDs,tri.primIDs,scene,geomID_to_instID)); 
      }
      
      static __forceinline bool occluded(const Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive& tri, Scene* scene, const unsigned* geomID_to_instID)
      {
        STAT3(shadow.trav_prims,1,1,1);
        Vec3<vfloat<M>> v0, v1, v2; tri.gather(v0,v1,v2);
        return pre.intersect(ray,v0,v1,v2,UVIdentity<Mx>(),Occluded1EpilogM<M,Mx,filter>(ray,context,tri.geomIDs,tri.primIDs,scene,geomID_to_instID)); 
      }

      /*! Intersect an array of rays with an array of M primitives. */
      static __forceinline size_t intersect(Precalculations* pre, size_t valid, Ray** rays, IntersectContext* context,  size_t ty, const Primitive* prim, size_t num, Scene* scene, const unsigned* geomID_to_instID)
      {
        size_t valid_isec = 0;
        do {
          const size_t i = __bscf(valid);
          const float old_far = rays[i]->tfar;
          for (size_t n=0; n<num; n++)
            intersect(pre[i],*rays[i],context,prim[n],scene,geomID_to_instID);
          valid_isec |= (rays[i]->tfar < old_far) ? ((size_t)1 << i) : 0;            
        } while(unlikely(valid));
        return valid_isec;
      }

    };
    
    /*! Intersects M triangles with K rays */
    template<int M, int Mx, int K, bool filter>
      struct TriangleMiIntersectorKPluecker
      {
        typedef TriangleMi<M> Primitive;
        typedef PlueckerIntersectorK<Mx,K> Precalculations;
        
        static __forceinline void intersect(const vbool<K>& valid_i, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const Primitive& tri, Scene* scene)
        {
          for (size_t i=0; i<Primitive::max_size(); i++)
          {
            if (!tri.valid(i)) break;
            STAT3(normal.trav_prims,1,popcnt(valid_i),RayK<K>::size());
            const Vec3f& p0 = *tri.v0[i];
            const Vec3f& p1 = *(Vec3f*)((int*)&p0 + tri.v1[i]);
            const Vec3f& p2 = *(Vec3f*)((int*)&p0 + tri.v2[i]);
            const Vec3<vfloat<K>> v0 = Vec3<vfloat<K>>(p0);
            const Vec3<vfloat<K>> v1 = Vec3<vfloat<K>>(p1);
            const Vec3<vfloat<K>> v2 = Vec3<vfloat<K>>(p2);
            pre.intersectK(valid_i,ray,v0,v1,v2,UVIdentity<K>(),IntersectKEpilogM<M,K,filter>(ray,context,tri.geomIDs,tri.primIDs,i,scene));
          }
        }
        
        static __forceinline vbool<K> occluded(const vbool<K>& valid_i, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const Primitive& tri, Scene* scene)
        {
          vbool<K> valid0 = valid_i;
          
          for (size_t i=0; i<Primitive::max_size(); i++)
          {
            if (!tri.valid(i)) break;
            STAT3(shadow.trav_prims,1,popcnt(valid_i),RayK<K>::size());
            const Vec3f& p0 = *tri.v0[i];
            const Vec3f& p1 = *(Vec3f*)((int*)&p0 + tri.v1[i]);
            const Vec3f& p2 = *(Vec3f*)((int*)&p0 + tri.v2[i]);
            const Vec3<vfloat<K>> v0 = Vec3<vfloat<K>>(p0);
            const Vec3<vfloat<K>> v1 = Vec3<vfloat<K>>(p1);
            const Vec3<vfloat<K>> v2 = Vec3<vfloat<K>>(p2);
            pre.intersectK(valid0,ray,v0,v1,v2,UVIdentity<K>(),OccludedKEpilogM<M,K,filter>(valid0,ray,context,tri.geomIDs,tri.primIDs,i,scene));
            if (none(valid0)) break;
          }
          return !valid0;
        }

        static __forceinline void intersect(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const Primitive& tri, Scene* scene)
        {
          STAT3(normal.trav_prims,1,1,1);
          Vec3<vfloat<M>> v0, v1, v2; tri.gather(v0,v1,v2);
          pre.intersect(ray,k,v0,v1,v2,UVIdentity<Mx>(),Intersect1KEpilogM<M,Mx,K,filter>(ray,k,context,tri.geomIDs,tri.primIDs,scene)); 
        }
        
        static __forceinline bool occluded(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const Primitive& tri, Scene* scene)
        {
          STAT3(shadow.trav_prims,1,1,1);
          Vec3<vfloat<M>> v0, v1, v2; tri.gather(v0,v1,v2);
          return pre.intersect(ray,k,v0,v1,v2,UVIdentity<Mx>(),Occluded1KEpilogM<M,Mx,K,filter>(ray,k,context,tri.geomIDs,tri.primIDs,scene)); 
        }
      };

      /*! Intersects M motion blur triangles with 1 ray */
      template<int M, int Mx, bool filter>
        struct TriangleMiMBIntersector1Pluecker
      {
        typedef TriangleMiMB<M> Primitive;
        typedef PlueckerIntersector1<Mx> Precalculations;

        /*! Intersect a ray with the M triangles and updates the hit. */
        static __forceinline void intersect(const Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive& tri, Scene* scene, const unsigned* geomID_to_instID)
        {
          STAT3(normal.trav_prims,1,1,1);
          Vec3<vfloat<M>> v0,v1,v2; tri.gather(v0,v1,v2,scene,context->itime,context->ftime);
          pre.intersect(ray,v0,v1,v2,UVIdentity<Mx>(),Intersect1EpilogM<M,Mx,filter>(ray,context,tri.geomIDs,tri.primIDs,scene,geomID_to_instID));
        }

        /*! Test if the ray is occluded by one of M triangles. */
        static __forceinline bool occluded(const Precalculations& pre, Ray& ray, IntersectContext* context, const Primitive& tri, Scene* scene, const unsigned* geomID_to_instID)
        {
          STAT3(shadow.trav_prims,1,1,1);
          Vec3<vfloat<M>> v0,v1,v2; tri.gather(v0,v1,v2,scene,context->itime,context->ftime);
          return pre.intersect(ray,v0,v1,v2,UVIdentity<Mx>(),Occluded1EpilogM<M,Mx,filter>(ray,context,tri.geomIDs,tri.primIDs,scene,geomID_to_instID));
        }
      };

      /*! Intersects M motion blur triangles with K rays. */
      template<int M, int Mx, int K, bool filter>
        struct TriangleMiMBIntersectorKPluecker
        {
          typedef TriangleMiMB<M> Primitive;
          typedef PlueckerIntersectorK<Mx,K> Precalculations;

          /*! Intersects K rays with M triangles. */
          static __forceinline void intersect(const vbool<K>& valid_i, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const TriangleMiMB<M>& tri, Scene* scene)
          {
            for (size_t i=0; i<TriangleMiMB<M>::max_size(); i++)
            {
              if (!tri.valid(i)) break;
              STAT3(normal.trav_prims,1,popcnt(valid_i),K);
              const Vec3<vfloat<K>> v0 = tri.getVertex(tri.v0,i,scene,0,ray.time);
              const Vec3<vfloat<K>> v1 = tri.getVertex(tri.v1,i,scene,0,ray.time);
              const Vec3<vfloat<K>> v2 = tri.getVertex(tri.v2,i,scene,0,ray.time);
              pre.intersectK(valid_i,ray,v0,v1,v2,UVIdentity<K>(),IntersectKEpilogM<M,K,filter>(ray,context,tri.geomIDs,tri.primIDs,i,scene));
            }
          }

          /*! Test for K rays if they are occluded by any of the M triangles. */
          static __forceinline vbool<K> occluded(const vbool<K>& valid_i, Precalculations& pre, RayK<K>& ray, IntersectContext* context, const TriangleMiMB<M>& tri, Scene* scene)
          {
            vbool<K> valid0 = valid_i;
            for (size_t i=0; i<TriangleMiMB<M>::max_size(); i++)
            {
              if (!tri.valid(i)) break;
              STAT3(shadow.trav_prims,1,popcnt(valid0),K);
              const Vec3<vfloat<K>> v0 = tri.getVertex(tri.v0,i,scene,0,ray.time);
              const Vec3<vfloat<K>> v1 = tri.getVertex(tri.v1,i,scene,0,ray.time);
              const Vec3<vfloat<K>> v2 = tri.getVertex(tri.v2,i,scene,0,ray.time);
              pre.intersectK(valid0,ray,v0,v1,v2,UVIdentity<K>(),OccludedKEpilogM<M,K,filter>(valid0,ray,context,tri.geomIDs,tri.primIDs,i,scene));
              if (none(valid0)) break;
            }
            return !valid0;
          }

          /*! Intersect a ray with M triangles and updates the hit. */
          static __forceinline void intersect(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const TriangleMiMB<M>& tri, Scene* scene)
          {
            STAT3(normal.trav_prims,1,1,1);
            Vec3<vfloat<M>> v0,v1,v2; tri.gather(v0,v1,v2,scene,0,ray.time[k]);
            pre.intersect(ray,k,v0,v1,v2,UVIdentity<Mx>(),Intersect1KEpilogM<M,Mx,K,filter>(ray,k,context,tri.geomIDs,tri.primIDs,scene));
          }

          /*! Test if the ray is occluded by one of the M triangles. */
          static __forceinline bool occluded(Precalculations& pre, RayK<K>& ray, size_t k, IntersectContext* context, const TriangleMiMB<M>& tri, Scene* scene)
          {
            STAT3(shadow.trav_prims,1,1,1);
            Vec3<vfloat<M>> v0,v1,v2; tri.gather(v0,v1,v2,scene,0,ray.time[k]);
            return pre.intersect(ray,k,v0,v1,v2,UVIdentity<Mx>(),Occluded1KEpilogM<M,Mx,K,filter>(ray,k,context,tri.geomIDs,tri.primIDs,scene));
          }
        };
  }
}
