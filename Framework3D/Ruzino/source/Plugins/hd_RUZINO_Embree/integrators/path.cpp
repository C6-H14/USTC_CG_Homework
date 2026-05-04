#include "path.h"
#include <random>
#include "../utils/math.hpp"
#include "../utils/sampling.hpp"
#include "../surfaceInteraction.h"

RUZINO_NAMESPACE_OPEN_SCOPE
using namespace pxr;

VtValue PathIntegrator::Li(const GfRay& ray, std::default_random_engine& random) {
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f - std::numeric_limits<float>::epsilon());
    std::function<float()> uniform_float = std::bind(uniform_dist, random);
    auto color = EstimateOutGoingRadiance(ray, uniform_float, 0);
    return VtValue(GfVec3f(color[0], color[1], color[2]));
}

GfVec3f PathIntegrator::EstimateOutGoingRadiance(const GfRay& ray, const std::function<float()>& uniform_float, int recursion_depth) {
    if (recursion_depth >= 50) return {};

    SurfaceInteraction si;
    if (!Intersect(ray, si)) {
        if (recursion_depth == 0) return IntersectDomeLight(ray);
        return GfVec3f{ 0, 0, 0 };
    }

    if (GfDot(si.shadingNormal, ray.GetDirection()) > 0) {
        si.flipNormal();
        si.PrepareTransforms();
    }

    GfVec3f globalLight = GfVec3f{ 0.f };
    GfVec3f directLight = GfVec3f{ 0.f };
    auto basis = constructONB(si.shadingNormal);

    // Strategy A: Light Sampling
    GfVec3f wi_light, sampled_light_pos;
    float pdf_light;
    Color light_radiance = SampleLights(si.position, wi_light, sampled_light_pos, pdf_light, uniform_float);
    
    if (pdf_light > 0.0f && VisibilityTest(si.position + 0.0001f * si.geometricNormal, sampled_light_pos)) {
        float cos_theta_light = GfMax(0.0f, GfDot(si.shadingNormal, wi_light));
        auto brdf_val_light = si.Eval(wi_light); 
        float pdf_brdf_at_light_dir = si.Pdf(wi_light, si.wo); 
        float mis_weight_light = (pdf_light * pdf_light) / (pdf_light * pdf_light + pdf_brdf_at_light_dir * pdf_brdf_at_light_dir + 1e-8f);
        directLight = GfCompMult(light_radiance, brdf_val_light) * cos_theta_light * mis_weight_light / pdf_light;
    }

    // Strategy B: BRDF Sampling
    float pdf_brdf;
    GfVec2f u(uniform_float(), uniform_float());
    GfVec3f local_dir = CosineWeightedDirection(u, pdf_brdf); 
    GfVec3f wi_brdf = basis * local_dir;

    if (pdf_brdf > 1e-6f) {
        float cos_theta_brdf = GfMax(0.0f, GfDot(si.shadingNormal, wi_brdf));
        auto brdf_val_brdf = si.Eval(wi_brdf);

        if (cos_theta_brdf > 0.0f) {
            GfRay next_ray(si.position + 0.0001f * si.geometricNormal, wi_brdf);
            float p_rr = (recursion_depth > 0) ? 0.8f : 1.0f;
            
            if (recursion_depth > 0 && uniform_float() > p_rr) {
                return directLight; // Russian Roulette Termination
            }

            SurfaceInteraction next_si;
            if (Intersect(next_ray, next_si)) {
                GfVec3f incoming_rad = EstimateOutGoingRadiance(next_ray, uniform_float, recursion_depth + 1);
                globalLight = GfCompMult(brdf_val_brdf, incoming_rad) * cos_theta_brdf / (pdf_brdf * p_rr);
            } else {
                Color hit_light_radiance = IntersectLights(next_ray, sampled_light_pos); 
                if (hit_light_radiance[0] > 0 || hit_light_radiance[1] > 0 || hit_light_radiance[2] > 0) {
                    float pdf_light_at_brdf_dir = 1.0f;
                    float mis_weight_brdf = (pdf_brdf * pdf_brdf) / (pdf_brdf * pdf_brdf + pdf_light_at_brdf_dir * pdf_light_at_brdf_dir + 1e-8f);
                    directLight += GfCompMult(hit_light_radiance, brdf_val_brdf) * cos_theta_brdf * mis_weight_brdf / (pdf_brdf * p_rr);
                }
            }
        }
    }
    return directLight + globalLight;
}
RUZINO_NAMESPACE_CLOSE_SCOPE