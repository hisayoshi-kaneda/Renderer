#pragma once

#ifndef FBO_H
#define FBO_H

#include "Texture2D.h"
#include "Texture2DArray.h"
#include "core/common.h"

class FrameBufferObject {
public:
    GLuint fboId = 0;
    int width = 0, height = 0;

    FrameBufferObject() {
        initialize();
    }

    ~FrameBufferObject() {
        if (fboId != 0) {
            glDeleteFramebuffers(1, &fboId);
        }
    }

    void setViewport(int width_, int height_) {
        width = width_;
        height = height_;
    }

    void attachColorTexture(Texture2D &tex) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.textureId, 0);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void attachColorTexture(Texture2DArray &texArray, const GLint layer) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texArray.textureArrayId, 0, layer);
    }

    void attachDepthTexture(Texture2D &tex) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex.textureId, 0);
    }

    void attachDepthTexture(Texture2DArray &texArray, const GLint layer) {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texArray.textureArrayId, 0, layer);
    }

    void bind() {
        if (width == 0 && height == 0) {
            fprintf(stderr, "Please set width and height of fbo.\n");
            exit(1);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glViewport(0, 0, width, height);
    }

    void release() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    void initialize() {
        glActiveTexture(GL_TEXTURE0);

        //Generate frame buffer
        glGenFramebuffers(1, &fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);

        //Release frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

#endif //FBO_H
