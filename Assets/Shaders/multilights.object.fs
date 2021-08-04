# version 460 core

// Result of fragment shader
out vec4 FragColor;

struct PhongModel 
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material 
{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

struct DirectionLight 
{
    vec3 direction;
};

struct PointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight 
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform PhongModel phong_model;
uniform DirectionLight directionLight;
uniform SpotLight spotLight;
uniform PointLight pointLight;
uniform Material material;

// uniform sampler2D texture_diffuse1;
// uniform sampler2D texture_specular1;

vec3 CalculateDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir);
vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() 
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = CalculateDirectionLight(directionLight, norm, viewDir);
    result += CalculatePointLight(pointLight, norm, FragPos, viewDir);
    result += CalculateSpotLight(spotLight, norm, FragPos, viewDir);

    // 正常
    FragColor = vec4(result, 1.0);

    // 反色
    FragColor = vec4(vec3(1.0, 1.0, 1.0) - result, 1.0);

    // 灰度
    float avg = (result.r + result.g + result.b) / 3.0;
    FragColor = vec4(avg, avg, avg, 1.0);

    // 加权平均后的灰度
    // float avg = 0.2126 * result.r + 0.7152 * result.g + 0.0722 * result.b;
    // FragColor = vec4(avg, avg, avg, 1.0);

    // Visualize the depth buffer
    //FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}

vec3 CalculateDirectionLight(DirectionLight light, vec3 normal, vec3 viewDir) 
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir,normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient = phong_model.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = phong_model.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = phong_model.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    return (ambient + diffuse + specular);
}

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = phong_model.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = phong_model.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = phong_model.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) 
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = phong_model.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = phong_model.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = phong_model.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

    ambient *= (attenuation * intensity);
    diffuse *= (attenuation * intensity);
    specular *= (attenuation * intensity);

    return (ambient + diffuse + specular);
    
}