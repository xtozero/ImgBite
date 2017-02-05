#include "pch.h"
#include "D3D12Renderer.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Platform;

ref class PaintTestApp sealed : public IFrameworkView
{
public:
	virtual void Initialize( CoreApplicationView^ CoreAppView )
	{
		CoreAppView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>( this, &PaintTestApp::OnActivated );
	}

	virtual void SetWindow( CoreWindow^ window )
	{
		window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>( this, &PaintTestApp::OnClosed );
	}

	virtual void Load( String^ entryPoint )
	{

	}

	virtual void Run( )
	{
		m_renderer.OnInitialize( );

		CoreWindow^ window = CoreWindow::GetForCurrentThread( );

		while ( m_windowClosed == false )
		{
			window->Dispatcher->ProcessEvents( CoreProcessEventsOption::ProcessAllIfPresent );
			m_renderer.OnRender( );
		}

		m_renderer.OnDestory( );
	}

	virtual void Uninitialize( )
	{
		
	}

private:
	void OnActivated( CoreApplicationView^ CoreAppView, IActivatedEventArgs^ Args )
	{
		CoreWindow^ window = CoreWindow::GetForCurrentThread( );
		window->Activate( );
	}

	void OnClosed( CoreWindow^ CoreAppView, CoreWindowEventArgs^ Args )
	{
		m_windowClosed = true;
	}

	CD3D12Renderer m_renderer;
	bool m_windowClosed = false;
};

ref class PaintTestAppSource sealed : public IFrameworkViewSource
{
public:
	virtual IFrameworkView^ CreateView( )
	{
		return ref new PaintTestApp( );
	}
};

[MTAThread]

int main( Array<String^>^ Args )
{
	CoreApplication::Run( ref new PaintTestAppSource( ) );
	return 0;
}