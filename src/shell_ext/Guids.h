#pragma once
#include <windows.h>
#include <initguid.h>

// ModelPeek Thumbnail Provider CLSID
// {B5A5C70A-7023-41E9-8CE9-B943B7B3E31A}
DEFINE_GUID(CLSID_ModelPeekThumbnailProvider,
    0xb5a5c70a, 0x7023, 0x41e9, 0x8c, 0xe9, 0xb9, 0x43, 0xb7, 0xb3, 0xe3, 0x1a);

// ModelPeek Preview Handler CLSID
// {8888E441-A88B-4B9F-8408-A4BD114B3E01}
DEFINE_GUID(CLSID_ModelPeekPreviewHandler,
    0x8888e441, 0xa88b, 0x4b9f, 0x84, 0x08, 0xa4, 0xbd, 0x11, 0x4b, 0x3e, 0x01);

// Windows built-in Preview Handler Surrogate Host (prevhost.exe) AppID
// {6d2b5079-2f0b-48dd-ab7f-97cec514d30b}
#define PREVHOST_APPID_STRING   L"{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}"
#define CLSID_THUMBNAIL_STRING  L"{B5A5C70A-7023-41E9-8CE9-B943B7B3E31A}"
#define CLSID_PREVIEW_STRING    L"{8888E441-A88B-4B9F-8408-A4BD114B3E01}"
