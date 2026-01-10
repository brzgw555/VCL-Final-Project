#include "Labs/3-Rendering/tasks.h"

namespace VCX::Labs::Rendering {

    glm::vec4 GetTexture(Engine::Texture2D<Engine::Formats::RGBA8> const & texture, glm::vec2 const & uvCoord) {
        if (texture.GetSizeX() == 1 || texture.GetSizeY() == 1) return texture.At(0, 0);
        glm::vec2 uv      = glm::fract(uvCoord);
        uv.x              = uv.x * texture.GetSizeX() - .5f;
        uv.y              = uv.y * texture.GetSizeY() - .5f;
        std::size_t xmin  = std::size_t(glm::floor(uv.x) + texture.GetSizeX()) % texture.GetSizeX();
        std::size_t ymin  = std::size_t(glm::floor(uv.y) + texture.GetSizeY()) % texture.GetSizeY();
        std::size_t xmax  = (xmin + 1) % texture.GetSizeX();
        std::size_t ymax  = (ymin + 1) % texture.GetSizeY();
        float       xfrac = glm::fract(uv.x), yfrac = glm::fract(uv.y);
        return glm::mix(glm::mix(texture.At(xmin, ymin), texture.At(xmin, ymax), yfrac), glm::mix(texture.At(xmax, ymin), texture.At(xmax, ymax), yfrac), xfrac);
    }

    glm::vec4 GetAlbedo(Engine::Material const & material, glm::vec2 const & uvCoord) {
        glm::vec4 albedo       = GetTexture(material.Albedo, uvCoord);
        glm::vec3 diffuseColor = albedo;
        return glm::vec4(glm::pow(diffuseColor, glm::vec3(2.2)), albedo.w);
    }

    /******************* 1. Ray-triangle intersection *****************/
    bool IntersectTriangle(Intersection & output, Ray const & ray, glm::vec3 const & p1, glm::vec3 const & p2, glm::vec3 const & p3) {
        // your code here
        // Moller Trumbore algorithm
        glm::vec3 edge21     = p2 - p1;
        glm::vec3 edge31     = p3 - p1;
        glm::vec3 ray_dir    = normalize(ray.Direction);
        glm::vec3 ray_ori    = ray.Origin;
        glm::vec3 p1_o       = ray_ori - p1;
        glm::vec3 n_31_cross = glm::cross(ray_dir, edge31);
        glm::vec3 n_21_cross = glm::cross(p1_o, edge21);
        double    det        = glm::dot(edge21, n_31_cross);
        if (det < 1e-4) {
            return false;
        } else {
            double u = glm::dot(p1_o, n_31_cross) / det;
            if (u <= 0 || u >= 1) {
                return false;
            }
            double v = glm::dot(ray_dir, n_21_cross) / det;
            if (v <= 0 || v >= 1 || u + v >= 1) {
                return false;
            }
            double t = glm::dot(ray_dir, (glm::vec3(u, u, u) * edge21 + glm::vec3(v, v, v) * edge31 - p1_o));
            output.t = t;
            output.u = u;
            output.v = v;
        }

        return true;
    }

    glm::vec3 RayTrace(const RayIntersector & intersector, Ray ray, int maxDepth, bool enableShadow) {
        glm::vec3 color(0.0f);
        glm::vec3 weight(1.0f);

        for (int depth = 0; depth < maxDepth; depth++) {
            auto rayHit = intersector.IntersectRay(ray);
            if (! rayHit.IntersectState) return color;
            const glm::vec3 pos       = rayHit.IntersectPosition;
            const glm::vec3 n         = rayHit.IntersectNormal;
            const glm::vec3 kd        = rayHit.IntersectAlbedo;
            const glm::vec3 ks        = rayHit.IntersectMetaSpec;
            const float     alpha     = rayHit.IntersectAlbedo.w;
            const float     shininess = rayHit.IntersectMetaSpec.w * 256;

            glm::vec3 result(0.0f);
            /******************* 2. Whitted-style ray tracing *****************/
            // your code here

            for (const Engine::Light & light : intersector.InternalScene->Lights) {
                glm::vec3 l;
                float     attenuation;
                /******************* 3. Shadow ray *****************/
                if (light.Type == Engine::LightType::Point) {
                    l           = light.Position - pos;
                    attenuation = 1.0f / glm::dot(l, l);
                    if (enableShadow) {
                        // your code here
                        auto hit = intersector.IntersectRay(Ray(pos, l));
                        if (hit.IntersectState == true && hit.IntersectAlbedo.w >= 0.2f) {
                            glm::vec3 shadow_pos = hit.IntersectPosition;
                            glm::vec3 shadow_dir = light.Position - shadow_pos;
                            if (glm::dot(shadow_dir, l) > 0) {
                                continue;
                            }
                        }
                    }
                } else if (light.Type == Engine::LightType::Directional) {
                    l           = light.Direction;
                    attenuation = 1.0f;
                    if (enableShadow) {
                        // your code here
                        auto hit = intersector.IntersectRay(Ray(pos, l));
                        if (hit.IntersectState == true && hit.IntersectAlbedo.w >= 0.2f) {
                            continue;
                        }
                    }
                }

                /******************* 2. Whitted-style ray tracing *****************/
                // your code here
                glm::vec3 norm_n         = glm::normalize(n);
                glm::vec3 norm_l         = glm::normalize(l);
                glm::vec3 norm_eye       = glm::normalize(ray.Direction);
                float     diffuse_angle  = glm::max(glm::dot(norm_n, norm_l), 0.0f);
                glm::vec3 diffuse_color  = kd * diffuse_angle * attenuation * light.Intensity;
                glm::vec3 norm_half      = glm::normalize(norm_l + norm_eye);
                float     specular_angle = glm::max(glm::dot(norm_n, norm_half), 0.0f);
                glm::vec3 specular_color = ks * pow(specular_angle, shininess) * attenuation * light.Intensity;
                result += diffuse_color + specular_color;
            }
            result += kd * intersector.InternalScene->AmbientIntensity;

            if (alpha < 0.9) {
                // refraction
                // accumulate color
                glm::vec3 R = alpha * glm::vec3(1.0f);
                color += weight * R * result;
                weight *= glm::vec3(1.0f) - R;

                // generate new ray
                ray = Ray(pos, ray.Direction);
            } else {
                // reflection
                // accumulate color
                glm::vec3 R = ks * glm::vec3(0.5f);
                color += weight * (glm::vec3(1.0f) - R) * result;
                weight *= R;

                // generate new ray
                glm::vec3 out_dir = ray.Direction - glm::vec3(2.0f) * n * glm::dot(n, ray.Direction);
                ray               = Ray(pos, out_dir);
            }
        }

        return color;
    }
} // namespace VCX::Labs::Rendering
