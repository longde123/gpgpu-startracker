#ifndef STATSPHASE_H
#define STATSPHASE_H

#include <stdio.h>

#include "phase.h"
#include "tga.h"

class test1
{
  int mtest;
};

class StatsPhase: public Phase
{
public:
    // Vertex and fragment shader files
    const char * mVertFilename;
    const char * mFragFilename;

    // Handle to a program object
    GLuint mProgramObject;

    struct
    {
        std::string filename;
        GLuint program;
        // Sampler location
        GLint s_fillLoc;

        // Uniform locations
        GLint u_texDimLoc;
        GLint u_passLoc;
        GLint u_factorLoc;

        // Attribute locations
        GLint  positionLoc;
        GLint  texCoordLoc;

    } mProgFill;

    struct
    {
        std::string filename;
        GLuint program;
        // Sampler locations
        GLint s_labelLoc;
        GLint s_fillLoc;
        GLint s_resultLoc;
        GLint s_origLoc;
        // Uniform locations
        GLint u_texDimLoc;
        GLint u_passLoc;
        GLint u_stageLoc;
        GLint u_savingOffsetLoc;
        GLint u_factorLoc;

        // Attribute locations
        GLint  positionLoc;
        GLint  texCoordLoc;
    } mProgCount;

    struct
    {
        std::string filename;
        GLuint program;
        // Sampler locations
        GLint s_labelLoc;
        GLint s_fillLoc;
        GLint s_resultLoc;
        GLint s_origLoc;
        // Uniform locations
        GLint u_texDimLoc;
        GLint u_passLoc;
        GLint u_stageLoc;
        GLint u_savingOffsetLoc;
        GLint u_factorLoc;
        // Attribute locations
        GLint  positionLoc;
        GLint  texCoordLoc;
    } mProgCentroid;

    int mWidth;
    int mHeight;

    // Vertices
    GLfloat mVertices[20];
    GLushort mIndices[6];

    // Texture handle
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaLabel;
    TGAData *mTgaReduced;
    TGAData *mTgaOrig;

    // Texture to attach to the frambuffers
    GLuint mTexOrigId;
    GLuint mTexLabelId;
    GLuint mTexReducedId;
    GLuint mTexFillId;
    GLuint mTexPiPoId[2];
    GLuint mFboId[2];
    GLint  mTextureUnits[6];

    int mWrite;
    int mRead;

    StatsPhase(int width = 0, int height = 0);
    virtual ~StatsPhase();
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

    void updateTextures(GLuint origTex   , GLint origTexUnit,
                        GLuint labelTex  , GLint labelTexUnit,
                        GLuint reducedTex, GLint reducedTexUnit,
                        GLuint freeTex   , GLint freeTexUnit,
                        GLuint freeTex2   , GLint freeTexUnit2);
//    GLint getLastTexture();
//    GLint getLastTexUnit();
//    GLint getFreeTexture();
//    GLint getFreeTexUnit();

    void setupGeometry();

    virtual double run();

private:
    void fillStage(float factorX, float factorY);
    void countStage(float factorX, float factorY, int offset);
    void centroidingStage(float factorX, float factorY, int coordinate, int offset);
};

#endif // STATSPHASE_H
