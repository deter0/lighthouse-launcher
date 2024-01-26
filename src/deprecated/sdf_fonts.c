// Deprecated because we only really have one font size so it is useless

Font load_sdf_font(const char *font_path) {
	Font fontSDF = { 0 };
	
	int font_file_size = 0;
	unsigned char *font_file_data = LoadFileData(font_path, &font_file_size);

	fontSDF.baseSize = 32;
	fontSDF.glyphCount = 95;
	
	fontSDF.glyphs = LoadFontData(font_file_data, font_file_size, fontSDF.baseSize, 0, 0, FONT_SDF);
	
	Image atlas = GenImageFontAtlas(fontSDF.glyphs, &fontSDF.recs, fontSDF.glyphCount, fontSDF.baseSize, 0, 1);
	fontSDF.texture = LoadTextureFromImage(atlas);
	
	UnloadImage(atlas);
	UnloadFileData(font_file_data);

	SetTextureFilter(fontSDF.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

	return fontSDF;
}

