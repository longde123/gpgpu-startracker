/*!
    \ingroup reduction
    @{
*/
varying vec2 v_texCoord;        /*!< texture coordinates of the current pixel */
uniform sampler2D s_texture;    /*!< Sampler holding the input image with intermediat results*/
uniform sampler2D s_values;     /*!< Sampler holding the labeled input image*/
uniform int u_stage;            /*!< The stage this algorithm is currently in */
uniform int u_pass;             /*!< The number of the pass this algorithm is in */
uniform int u_direction;        /*!< The direction of the algorithm i.e. vertical or horizontal */

/*!
 * Reduction shader
 * @author Jan Sommer
 * @date 2014
 * @namespace GLSL
 * @class reductionShader
 */

#define RUNNING_SUM     0
#define BINARY_SEARCH   1
#define ROOT_INIT       2

#define HORIZONTAL      0
#define VERTICAL        1

void runningSum()
{
    vec2 coord ;
    float withinBounds;
    if(u_direction == HORIZONTAL)
    {
        coord = tex2imgCoord(v_texCoord) - vec2(exp2( float(u_pass) ), ZERO);
        withinBounds = step(ZERO, coord.x+0.5 );
    }
    else // VERTICAL
    {
        coord = tex2imgCoord(v_texCoord) - vec2( ZERO, exp2( float(u_pass) ) );
        withinBounds = step(ZERO, coord.y+0.5 );
    }

    coord = img2texCoord( coord );

    if(u_pass == 0)
    {
        vec4 pixelCol = texture2D( s_values, v_texCoord );
        vec2 curLabel = unpack2shorts(pixelCol);

        // count == 1.0 if curPixelCol is zero and 0.0 otherwise
        float count = ONE - step(ONE/256.0, length(pixelCol) );

        // Check if pixel to the left is zero
        pixelCol = texture2D( s_values, coord);
        curLabel = unpack2shorts(pixelCol);

        // Add 1.0 to count if pixel to the left is zero, make sure not to read outside of texture
        count += ( ONE - step(ONE/256.0, length(pixelCol) ) ) * withinBounds;

        // Save the number of zero pixels from this and left neighbor pixel (0.0, 1.0 or 2.0)
        gl_FragColor = pack2shorts(vec2(count, ZERO));
    }
    else if(u_pass > 0)
    // Add up the zeroes for all pixels to the left
    {
        vec4 pixelCol = texture2D( s_texture, v_texCoord );
        float count = unpack2shorts(pixelCol).x;

        pixelCol = texture2D( s_texture, coord );

        // Add the value 2^u_pass to the y-component (filter out values outside of texture range)
        count += unpack2shorts(pixelCol).x * withinBounds;
        gl_FragColor = pack2shorts(vec2(count, exp2( float(u_pass) )) );

    }
}

void binarySearch()
{
    // 1. Get the last guess for the current texel
    // For the very first time it will be 2^u_pass (result from the previous stage)
    // Note that u_pass counts backwards from max u_pass of previous stage
    vec2 current = unpack2shorts(texture2D(s_texture, v_texCoord ) );
    float lastGuess = current.y;
    vec2  coord;
    if(u_direction == HORIZONTAL)
        coord = tex2imgCoord(v_texCoord) + vec2(lastGuess, ZERO);
    else // VERTICAL
        coord = tex2imgCoord(v_texCoord) + vec2(ZERO, lastGuess);

    if(u_pass > 0)
    // gather search after scan
    {
        /*
           Find the texel which running sum is equal its distance to the current texel
           by using a binary search:
        */
        //   2. Get the value of the current guess
        coord = img2texCoord(coord);
        float guess = unpack2shorts( texture2D(s_texture, coord ) ).x;
        vec2  value = unpack2shorts( texture2D(s_values,  coord ) );

        // Version with if:
        float outGuess;

        if(guess > lastGuess)
        {
            outGuess = lastGuess + exp2(float(u_pass)-ONE);
        }
        else if(guess == lastGuess && length(value) > ZERO)
        {
            outGuess = lastGuess;
        }
        else
        {
            outGuess = lastGuess - exp2(float(u_pass)-ONE);
        }

        gl_FragColor = pack2shorts( vec2(current.x, outGuess) );

        // Version without if:
//        float factor = TWO * float(guess > lastGuess) - ONE + float(guess==lastGuess) * step(ONE/256.0, length(value) ) ;
//        gl_FragColor = pack2shorts( vec2( current.x, lastGuess+factor*exp2(-float(u_pass)-TWO) ) );



    }
    else if(u_pass == 0)
    {
        coord = img2texCoord(coord);

        float guess = unpack2shorts( texture2D(s_texture, coord ) ).x;
        vec2  value = unpack2shorts( texture2D(s_values,  coord ) );

        // Version with if:
        float outGuess;


        if(guess > lastGuess)
        {
            outGuess = lastGuess + ONE;
        }
        else if(guess == lastGuess && length(value) > ZERO)
        {
            outGuess = lastGuess;
        }
        else
        {
            outGuess = lastGuess - ONE;
        }

        outGuess = floor(outGuess+0.5);

        // Final assignment
        float withinBounds;
        if(u_direction == HORIZONTAL)
        {
            coord = tex2imgCoord(v_texCoord) + vec2(outGuess, ZERO);
            withinBounds = float( floor((u_texDimensions.x-coord.x)) > ZERO );
        }
        else // VERTICAL
        {
            coord = tex2imgCoord(v_texCoord) + vec2(ZERO, outGuess);
            withinBounds = float( floor((u_texDimensions.y-coord.y)) > ZERO );
        }
        coord = img2texCoord(coord);

        gl_FragColor = texture2D(s_values, coord) * withinBounds;
    }
}

/*!
  \brief Main program of the reduction shader

  A more detailed explanation of the algorithm can be found at \ref reduction
  The reduction shader has basically 3 different stages:

  Root init stage
  ----------------

  The input texture with the labeled image (result of labeling phase) is
  available in \ref s_values. Each pixels samples the labels and checks if it
  is a root-pixel, i.e. a pixel which has the same image coordinates as
  the coordinates in its respective label. If yes, it will write its label
  to the output buffer, otherwise 0.

  Running sum stage
  -----------------

  The result of the running sum stage is that each individual pixel
  has the number of zero-pixels to the left (top) in its row (column).
  The sum will be monotonic ascending from left to right (top to bottom).

  Basically in the end each pixel will "know" how many pixels it has to
  move to the right (top) in order to reduce.


  Binary search stage
  -------------------

  Each pixel searches for a pixel which has a sum (from previous stage) which
  is equal to the distance of that pixel from the current pixel.
  As the sum is monotonic ascending a binary search algorithm is used starting from
  the most right (bottom) pixel.

*/
void main()
{
    if(u_stage == RUNNING_SUM)
    {
        runningSum();
    }

    else if(u_stage == BINARY_SEARCH)
    {
        binarySearch();
    }
    else if(u_stage == ROOT_INIT)
    {
        vec4 curColor = texture2D(s_values, v_texCoord );
        bool isRoot = all( equal( floor(unpack2shorts(curColor)), floor(tex2imgCoord(v_texCoord)+0.5) + ONE ) );
        gl_FragColor = curColor * float(isRoot);
    }
}
