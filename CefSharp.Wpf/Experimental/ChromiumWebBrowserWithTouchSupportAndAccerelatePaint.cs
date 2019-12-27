using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CefSharp.Wpf.Rendering.Experimental;

namespace CefSharp.Wpf.Experimental
{
    public class ChromiumWebBrowserWithTouchSupportAndAccerelatePaint : ChromiumWebBrowserWithTouchSupport
    {
        public ChromiumWebBrowserWithTouchSupportAndAccerelatePaint() : base()
        {
            RenderHandler = new AcceleratedPaintRenderHandler();
        }

        protected override IWindowInfo CreateOffscreenBrowserWindowInfo(IntPtr handle)
        {
            var windowInfo = new WindowInfo();
            windowInfo.SetAsWindowless(handle);
            windowInfo.ExternalBeginFrameEnabled = true;
            windowInfo.SharedTextureEnabled = true;
            return windowInfo;
        }
    }
}
