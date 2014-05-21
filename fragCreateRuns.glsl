precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform int u_pass;
uniform float u_threshold;      // threshold value for the threshold operation

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts /= 256.0f;
    return vec4(floor(shorts)/255.0f, fract(shorts)*256.0f/255.0f).zxwy;
}

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function unpacks 2 16-bit short integer values from the 4 texture channels into 2 floats.

in vec4:  RGBA value to unpack with LSB first

return vec2: vector which will contain the 2 shorts

*/
vec2 unpack2shorts(in vec4 rgba)
{
    // LSB * 255 + MSB * 255*256
    return vec2(rgba.xz * 255.0f + 255.0f*256.0f * rgba.yw);
}

/*
This functions computes the image coordinates of the texture with the dimensions from u_texDimensions
Example:

For an 8 pixel texture (8x1) the texture coordinates are in [0.0, 1.0].
glNearest will yield texture coordinates which are  in the center of each pixel (e.g. 1/16, 3/16 etc).

 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 ^   ^   ^   ^   ^   ^   ^   ^   ^
0.0  |   |   |   |   |   |   |  1.0
 |   |   |   |   |   |   |   |   |
0/8 1/8 2/8 3/8 4/8 5/8 6/8 7/8 8/8

Therefore, in order to get the pixel coordinates the following formular is used:

px = (2*tx*dimX-1)/2
py = (2*ty*dimY-1)/2

in vec2: vector with texture coordinates
return vec2: vecture with image coordinates

*/
vec2 tex2imgCoord(in vec2 texCoord)
{
    return (2.0f*texCoord*u_texDimensions-1.0f)/2.0f;
}


/*
Reverse function of tex2imgCoord. Takes image coordinates and
return the  corresponding texture coordinates.

tx = (2*px+1)/2dimX
ty = (2*py+1)/2dimY

in vec2: image coordinates in [0, width], [0, height]
return vec2: texture coordinates in [0,1], [0,1]
*/
vec2 img2texCoord(in vec2 imgCoord)
{
    return (2.0f*imgCoord + 1.0f)/(2.0f*u_texDimensions);
}

void main()
{
    if (u_pass == 0)
    {
        vec4 curPixelCol = texture2D( s_texture, v_texCoord ).rrrr;
        // Threshold operation
        curPixelCol = step(u_threshold, curPixelCol);

        vec2 curPixelCoord = gl_FragCoord.xy * curPixelCol.xy;

        // Get neighbor pixel
        vec2 neighborPixelCoord = tex2imgCoord(v_texCoord) + vec2(0.0f, 1.0f);
        vec4 neighborCol = texture2D(s_texture, img2texCoord(neighborPixelCoord) ).rrrr;
        neighborCol = step(u_threshold, neighborCol);

        neighborPixelCoord = neighborPixelCoord * neighborCol.xy;

        mat2 coord = mat2(curPixelCoord, neighborPixelCoord);

        // Find out non-zero pixel with highest y value (bottom most)
        vec2 yValues = vec2(curPixelCoord.y, neighborPixelCoord.y);
        neighborPixelCoord = coord * step( max(yValues.x, yValues.y), yValues) ;


        gl_FragColor = neighborCol;

        gl_FragColor = pack2shorts(neighborPixelCoord) * curPixelCol;

    }
    else
    {
        vec2 curPixel = unpack2shorts( texture2D(s_texture, v_texCoord) );
        gl_FragColor  = texture2D(s_texture, img2texCoord(curPixel) );
//        gl_FragColor = texture2D(s_texture, v_texCoord);
    }
}
