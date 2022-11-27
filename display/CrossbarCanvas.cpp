#include "CrossbarCanvas.h"

CrossbarCanvas::CrossbarCanvas(IAshitaCore* pAshitaCore, CrossbarSettings* pSettings, CrossbarBindings* pBindings)
    : pAshitaCore(pAshitaCore)
    , pBindings(pBindings)
    , pSettings(pSettings)
{
    pMainDisplay = new GdiDIB(pAshitaCore->GetDirect3DDevice(), (pSettings->pSubPanel->PanelWidth * 2) + pSettings->pSubPanel->PanelSpacing, pSettings->pSubPanel->PanelHeight);
    pMainDisplay->GetGraphics()->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    pMainDisplay->GetGraphics()->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    pMainDisplay->GetGraphics()->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    pMainPrimitive = pAshitaCore->GetPrimitiveManager()->Create("CrossbarMain");
    pSubDisplay = new GdiDIB(pAshitaCore->GetDirect3DDevice(), pSettings->pSubPanel->PanelWidth, pSettings->pSubPanel->PanelHeight);
    pSubDisplay->GetGraphics()->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    pSubDisplay->GetGraphics()->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    pSubDisplay->GetGraphics()->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    pSubPrimitive = pAshitaCore->GetPrimitiveManager()->Create("CrossbarSub");
    pPaletteDisplay = new GdiDIB(pAshitaCore->GetDirect3DDevice(), pSettings->pSubPanel->PaletteWidth, pSettings->pSubPanel->PaletteHeight);
    pPaletteDisplay->GetGraphics()->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    pPaletteDisplay->GetGraphics()->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    pPaletteDisplay->GetGraphics()->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    pPalettePrimitive = pAshitaCore->GetPrimitiveManager()->Create("CrossbarPalette");
    mLastSingleMode = MacroMode::NoTrigger;
    
    RECT rect;
    GetWindowRect(pAshitaCore->GetProperties()->GetFinalFantasyHwnd(), &rect);

    if (pSettings->mConfig.MainPanelX == -1)
    {
        pMainPrimitive->SetPositionX((rect.right - rect.left - ((pSettings->pSubPanel->PanelWidth * 2) + pSettings->pSubPanel->PanelSpacing)) / 2);
    }
    else
    {
        pMainPrimitive->SetPositionX(pSettings->mConfig.MainPanelX);
    }
    if (pSettings->mConfig.MainPanelY < 0)
    {
        pMainPrimitive->SetPositionY((rect.bottom - rect.top) + pSettings->mConfig.MainPanelY - pSettings->pSubPanel->PanelHeight);
    }
    else
    {
        pMainPrimitive->SetPositionY(pSettings->mConfig.MainPanelY);
    }

    if (pSettings->mConfig.SubPanelX == -1)
    {
        pSubPrimitive->SetPositionX((rect.right - rect.left - pSettings->pSubPanel->PanelWidth) / 2);
    }
    else
    {
        pSubPrimitive->SetPositionX(pSettings->mConfig.SubPanelX);
    }
    if (pSettings->mConfig.SubPanelY < 0)
    {
        pSubPrimitive->SetPositionY((rect.bottom - rect.top) + pSettings->mConfig.SubPanelY - pSettings->pSubPanel->PanelHeight);
    }
    else
    {
        pSubPrimitive->SetPositionY(pSettings->mConfig.SubPanelY);
    }

    pPalettePrimitive->SetPositionX(pMainPrimitive->GetPositionX() + pSettings->pSubPanel->PanelWidth - (pSettings->pSubPanel->PaletteWidth/2) + (pSettings->pSubPanel->PanelSpacing / 2));
    pPalettePrimitive->SetPositionY(pMainPrimitive->GetPositionY() + pSettings->pSubPanel->PanelHeight);
    PaletteRect = Gdiplus::Rect(pSettings->pSubPanel->PaletteWidth/2, 0, pPalettePrimitive->GetWidth(), pPalettePrimitive->GetHeight());

    for (int x = 0; x < 6; x++)
    {
        pMacros[x] = new CrossbarMacroSet(pAshitaCore, pSettings, pBindings, (MacroMode)x);
    }
}
CrossbarCanvas::~CrossbarCanvas()
{
    pAshitaCore->GetPrimitiveManager()->Delete("CrossbarSub");
    delete pSubDisplay;
    pAshitaCore->GetPrimitiveManager()->Delete("CrossbarMain");
    delete pMainDisplay;
    pAshitaCore->GetPrimitiveManager()->Delete("CrossbarPalette");
    delete pPaletteDisplay;
    for (int x = 0; x < 6; x++)
    {
        delete pMacros[x];
    }
}

void CrossbarCanvas::Draw(MacroMode mode, IAshitaCore* m_AshitaCore)
{
    // Hide our palette if enough time passed
    if (pSettings->mInput.PaletteDelay > 0 && pPalettePrimitive->GetVisible())
    {
        if (std::chrono::steady_clock::now() >= mPaletteTimer)
        {
            pPalettePrimitive->SetVisible(false);
        }
    }

    if (pBindings->mRedraw)
    {
        UpdatePalette();
        pBindings->mRedraw = false;

        // Draw our palette primitive (only responsible for showing the name of the palette we are on)
        if (pBindings->pJobSettings->mPaletteList.size() > 1)
        {
            wchar_t textBuffer[1024];
            swprintf_s(textBuffer, 1024, L"%S", (*pBindings->pJobSettings->mPaletteIterator)->Name);
            if (wcscmp(textBuffer, PaletteText) != 0)
            {
                wcsncpy_s(PaletteText, textBuffer, 1024);
                if (PalettePath)
                {
                    delete PalettePath;
                }
                PalettePath = new Gdiplus::GraphicsPath();
                pSettings->pMacro->pPalette->AddToPath(PalettePath, PaletteText, PaletteRect);
            }
            pPaletteDisplay->ClearRegion(0, 0, pPalettePrimitive->GetWidth(), pPalettePrimitive->GetHeight());
            pPaletteDisplay->GetGraphics()->DrawPath(pSettings->pMacro->pPalette->pFont->pPen, PalettePath);
            pPaletteDisplay->GetGraphics()->FillPath(pSettings->pMacro->pPalette->pFont->pBrush, PalettePath);
            pPaletteDisplay->ApplyToPrimitiveObject(pPalettePrimitive);
            pPalettePrimitive->SetVisible(true);

            // Start a timer if we want to hide our palette after x amount of time
            if (pSettings->mInput.PaletteDelay > 0)
            {
                mPaletteTimer = std::chrono::steady_clock::now() + std::chrono::milliseconds(pSettings->mInput.PaletteDelay);
            }
        }
        else
        {
            pPalettePrimitive->SetVisible(false);
        }
    }

    // Tell our macrosets if they are being triggered
    for (int x = 0; x < 6; x++)
    {
        if (mode == (MacroMode)x)
        {
            pMacros[x]->ToggleTrigger(true);
        }
        else
        {
            pMacros[x]->ToggleTrigger(false);
        }
    }

    if ((mode == MacroMode::LeftTrigger) || (mode == MacroMode::RightTrigger) || (mode == MacroMode::NoTrigger))
    {
        pSubPrimitive->SetVisible(false);
       bool reapply = pMacros[(int)MacroMode::LeftTrigger]->Draw(pMainDisplay);
       if (pMacros[(int)MacroMode::RightTrigger]->Draw(pMainDisplay) || reapply)
        {
            pMainDisplay->ApplyToPrimitiveObject(pMainPrimitive);
        }
        pMainPrimitive->SetVisible(true);
    }
    else
    {
        if (mLastSingleMode != mode)
        {
            pMacros[(int)mode]->ForceMacroRedraw();
            mLastSingleMode = mode;
        }

        pMainPrimitive->SetVisible(false);
        if (pMacros[(int)mode]->Draw(pSubDisplay))
        {
            pSubDisplay->ApplyToPrimitiveObject(pSubPrimitive);
        }
        pSubPrimitive->SetVisible(true);
    }
}
void CrossbarCanvas::HandleButton(MacroButton button, MacroMode mode)
{
    pMacros[(int)mode]->TriggerMacro(button);
}
void CrossbarCanvas::Hide()
{
    pMainPrimitive->SetVisible(false);
    pSubPrimitive->SetVisible(false);
    pPalettePrimitive->SetVisible(false);
}
void CrossbarCanvas::UpdateBindings(CrossbarBindings* pNewBindings)
{
    delete pBindings;
    pBindings = pNewBindings;
    for (int x = 0; x < 6; x++)
    {
        delete pMacros[x];
        pMacros[x] = new CrossbarMacroSet(pAshitaCore, pSettings, pBindings, (MacroMode)x);
    }
}
void CrossbarCanvas::UpdatePalette()
{
    for (int x = 0; x < 6; x++)
    {
        delete pMacros[x];
        pMacros[x] = new CrossbarMacroSet(pAshitaCore, pSettings, pBindings, (MacroMode)x);
    }
}