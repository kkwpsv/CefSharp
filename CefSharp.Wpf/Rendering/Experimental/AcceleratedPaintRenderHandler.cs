using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using CefSharp.Wpf.D3D11Renderer;
using Rect = CefSharp.Structs.Rect;

namespace CefSharp.Wpf.Rendering.Experimental
{
    public class AcceleratedPaintRenderHandler : IRenderHandler
    {
        IntPtr lastTexture = IntPtr.Zero;
        double lastScale = 0;
        Renderer renderer;

        public AcceleratedPaintRenderHandler()
        {
            renderer = new Renderer();
            if (!renderer.Init())
            {
                throw new Exception("Init renderer failed");
            }
        }


        public void Dispose()
        {
        }

        public void OnAcceleratedPaint(bool isPopup, Rect dirtyRect, IntPtr sharedHandle, Image image)
        {
            image.Dispatcher.InvokeAsync(() =>
            {
                try
                {
                    var d3D11Image = image.Source as D3D11Image;
                    var scale = PresentationSource.FromVisual(image).CompositionTarget.TransformToDevice.M11;
                    if (lastScale != scale || d3D11Image == null)
                    {
                        d3D11Image = new D3D11Image(scale * 96, scale * 96)
                        {
                            WindowOwner = (PresentationSource.FromVisual(image) as HwndSource).Handle,
                            OnRender = renderer.Render,
                        };
                        image.Source = d3D11Image;
                        lastScale = scale;
                    }

                    if (sharedHandle != lastTexture && renderer.SetTexture(sharedHandle, out var width, out var height))
                    {
                        lastTexture = sharedHandle;
                        d3D11Image.SetPixelSize(width, height);
                    }


                    d3D11Image.RequestRender(new Int32Rect(0, 0, d3D11Image.PixelWidth, d3D11Image.PixelHeight));
                }
                catch
                {

                }
            });

        }


        public void OnPaint(bool isPopup, Rect dirtyRect, IntPtr buffer, int width, int height, Image image)
        {

        }
    }
}
