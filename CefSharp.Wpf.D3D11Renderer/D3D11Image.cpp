#include "pch.h"
#include "D3D11Image.h"

namespace CefSharp
{
    namespace Wpf
    {
        namespace D3D11Renderer
        {
            static D3D11Image::D3D11Image()
            {
                OnRenderProperty = DependencyProperty::Register("OnRender",
                    Action<IntPtr, bool>::typeid,
                    D3D11Image::typeid,
                    gcnew UIPropertyMetadata(nullptr, gcnew PropertyChangedCallback(&RenderChanged)));

                WindowOwnerProperty = DependencyProperty::Register("WindowOwner",
                    IntPtr::typeid,
                    D3D11Image::typeid,
                    gcnew UIPropertyMetadata(IntPtr::Zero, gcnew PropertyChangedCallback(&HWNDOwnerChanged)));
            }

            D3D11Image::D3D11Image()
            {
            }

            D3D11Image::D3D11Image(double dpiX, double dpiY) : D3DImage(dpiX, dpiY)
            {

            }

            D3D11Image::~D3D11Image()
            {
                if (this->Helper != nullptr)
                {
                    this->Helper->~SurfaceQueueInteropHelper();
                    this->Helper = nullptr;
                }
            }

            Freezable^ D3D11Image::CreateInstanceCore()
            {
                return gcnew D3D11Image();
            }

            void D3D11Image::HWNDOwnerChanged(DependencyObject^ sender, DependencyPropertyChangedEventArgs args)
            {
                D3D11Image^ image = dynamic_cast<D3D11Image^>(sender);

                if (image != nullptr)
                {
                    if (image->Helper != nullptr)
                    {
                        image->Helper->HWND = static_cast<IntPtr>(args.NewValue);
                    }
                }
            }

            void D3D11Image::RenderChanged(DependencyObject^ sender, DependencyPropertyChangedEventArgs args)
            {
                D3D11Image^ image = dynamic_cast<D3D11Image^>(sender);

                if (image != nullptr)
                {
                    if (image->Helper != nullptr)
                    {
                        image->Helper->RenderD2D = static_cast<Action<IntPtr, bool>^>(args.NewValue);
                    }
                }
            }

            void D3D11Image::EnsureHelper()
            {
                if (this->Helper == nullptr)
                {
                    this->Helper = gcnew SurfaceQueueInteropHelper();
                    this->Helper->HWND = this->WindowOwner;
                    this->Helper->D3DImage = this;
                    this->Helper->RenderD2D = this->OnRender;
                }
            }

            void D3D11Image::RequestRender(Int32Rect dirtyRect)
            {
                this->EnsureHelper();

                // Don't bother with a call if there's no callback registered.
                if (this->OnRender != nullptr)
                {
                    this->Helper->RequestRenderD2D(dirtyRect);
                }
            }

            void D3D11Image::SetPixelSize(int pixelWidth, int pixelHeight)
            {
                this->EnsureHelper();
                this->Helper->SetPixelSize(static_cast<UInt32>(pixelWidth), static_cast<UInt32>(pixelHeight));
            }
        }
    }
}

