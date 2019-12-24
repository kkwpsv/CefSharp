#pragma once
#include "RendererImp.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace CefSharp
{
    namespace Wpf
    {
        namespace D3D11Renderer
        {
            public ref class Renderer
            {
            public:
                Renderer()
                {
                    _renderImp = new RendererImp();
                }

                ~Renderer()
                {
                    this->!Renderer();
                }

                !Renderer()
                {
                    delete _renderImp;
                }

                bool Init()
                {
                    return _renderImp->Init();
                }

                bool SetTexture(IntPtr sharedHandle, [Out]int% width, [Out]int% height)
                {
                    int w = 0;
                    int h = 0;
                    bool result = _renderImp->SetTexture(sharedHandle.ToPointer(), w, h);
                    width = w;
                    height = h;
                    return result;
                }

                void Render(IntPtr surface, bool isNewSurface)
                {
                    _renderImp->Render(surface.ToPointer(), isNewSurface);
                }

            private:
                RendererImp* _renderImp;
            };
        }
    }
}
