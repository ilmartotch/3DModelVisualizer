// Nel metodo setOverrideTexture, assicurati di pulire il colore
void SceneObject::setOverrideTexture(GLuint texID) {
    overrideTextureID = texID;
    hasOverrideTexture_ = true;
    // IMPORTANTE: Quando imposti una texture, rimuovi il colore override
    clearOverrideColor();
}

void SceneObject::setOverrideColor(const glm::vec4& color) {
    overrideColor = color;
    hasOverrideColor_ = true;
    // IMPORTANTE: Quando imposti un colore, rimuovi la texture override
    clearOverrideTexture();
}

void SceneObject::clearOverrideTexture() {
    if (hasOverrideTexture_ && overrideTextureID != 0) {
        // Non eliminare la texture qui, potrebbe essere condivisa
        // Il TextureManager si occuperà della pulizia
    }
    overrideTextureID = 0;
    hasOverrideTexture_ = false;
}

void SceneObject::clearOverrideColor() {
    overrideColor = glm::vec4(1.0f);
    hasOverrideColor_ = false;
}