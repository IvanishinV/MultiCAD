#include "pch.h"
#include "ScreenConfig.h"

S32 Screen::width_  = Graphics::kDefaultWidth;
S32 Screen::height_ = Graphics::kDefaultHeight;
S32 Screen::widthInBytes_ = Screen::width_ * sizeof(Pixel);
S32 Screen::heightInBytes_ = Screen::height_ * sizeof(Pixel);
S32 Screen::sizeInPixels_ = Screen::width_ * Screen::height_;
S32 Screen::sizeInBytes_ = Screen::sizeInPixels_ * sizeof(Pixel);
S32 Screen::sizeInDoublePixels_ = Screen::sizeInPixels_ * sizeof(Pixel) * 2;

bool Screen::resolutionFromIni_ = false;
bool Screen::targetResolved_ = false;
S32 Screen::targetWidth_ = Graphics::kDefaultWidth;
S32 Screen::targetHeight_ = Graphics::kDefaultHeight;