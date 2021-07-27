# version 330 core

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 object_color;
uniform vec3 light_color;
//uniform vec3 light_pos;
uniform vec3 view_pos;

struct Light {
    vec3 position;

    float constant;
    float linear;
    float quadratic;
    
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutoff;
    float outerCutoff;
};

uniform Light light;

struct Material {
    // vec3 ambient;  ambient always equal to diffuse
    sampler2D diffuse;
    // vec3 specular;
    sampler2D specular;
    float shininess;
};

uniform Material material;

void main() 
{
    vec3 light_dir = normalize(light.position - FragPos);

    float theta = dot(light_dir, normalize(-light.direction));

    float epsilon = light.cutoff - light.outerCutoff;

    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    // if (theta > light.cutoff) 
    // {
        // Ambient
        // vec3 ambient = light.ambient * material.ambient * light_color;
        vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
        // vec3 result = (ambient + diffuse + specular) * object_color;

        // Diffuse 
        vec3 norm = normalize(Normal);
        
        float diff = max(dot(norm, light_dir), 0.0);
        // vec3 diffuse = light.diffuse * light_color * (diff * material.diffuse);
        vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

        // vec3 light_dir = -light.direction;
        
        // Specular
        vec3 view_dir = normalize(view_pos - FragPos);
        vec3 reflect_dir = reflect(-light_dir, norm);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
        // vec3 specular = light.specular * light_color * (spec * material.specular);
        vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

        
        // Dot Light Calc
        float distance = length(light.position - FragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

        // ambient *= attenuation;
        specular *= intensity;
        diffuse *= intensity;

        // FragColor = vec4(result, 1.0);
        FragColor = vec4(ambient + diffuse + specular, 1.0);
    // }
    // else
    // {
    //     FragColor = vec4(light.ambient * texture(material.diffuse, TexCoords).rgb, 1.0);
    // }
}