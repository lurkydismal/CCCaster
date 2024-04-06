#include "imgui.h"

#include "imgui_stdlib.h"

extern "C" uint16_t __declspec( dllexport )
    ImGui$Button( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const char** _buttonLabel =
        reinterpret_cast< const char** >( _callbackArguments[ 0 ] );
    const ImVec2* _buttonSize =
        reinterpret_cast< const ImVec2* >( _callbackArguments[ 1 ] );

    if ( ImGui::Button( *_buttonLabel, *_buttonSize ) ) {
        l_returnValue = 1;
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$InputText( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const char** _inputTextLabel =
        reinterpret_cast< const char** >( _callbackArguments[ 0 ] );
    std::string** _inputStringReference =
        reinterpret_cast< std::string** >( _callbackArguments[ 1 ] );

    if ( ImGui::InputText( *_inputTextLabel, *_inputStringReference ) ) {
        l_returnValue = 5;
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$Selectable( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const char** _selectableLabel =
        reinterpret_cast< const char** >( _callbackArguments[ 0 ] );
    bool* _isSelected = reinterpret_cast< bool* >( _callbackArguments[ 1 ] );
    ImGuiSelectableFlags* _selectableFlags =
        reinterpret_cast< ImGuiSelectableFlags* >( _callbackArguments[ 2 ] );
    const ImVec2* _selectableSize =
        reinterpret_cast< const ImVec2* >( _callbackArguments[ 3 ] );

    if ( ImGui::Selectable( *_selectableLabel, *_isSelected, *_selectableFlags,
                            *_selectableSize ) ) {
        l_returnValue = 1;
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$SameLine( void** _callbackArguments ) {
    float* _offsetFromStartX =
        reinterpret_cast< float* >( _callbackArguments[ 0 ] );
    float* _spacingBefore =
        reinterpret_cast< float* >( _callbackArguments[ 1 ] );

    ImGui::SameLine( *_offsetFromStartX, *_spacingBefore );

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$Indent( void** _callbackArguments ) {
    float* _indentWidth = reinterpret_cast< float* >( _callbackArguments[ 0 ] );

    ImGui::Indent( *_indentWidth );

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$BeginGroup( void** _callbackArguments ) {
    ImGui::BeginGroup();

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$EndGroup( void** _callbackArguments ) {
    ImGui::EndGroup();

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$BeginChild( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    ImGuiID* _childId = reinterpret_cast< ImGuiID* >( _callbackArguments[ 0 ] );
    const ImVec2* _childSize =
        reinterpret_cast< const ImVec2* >( _callbackArguments[ 1 ] );
    ImGuiChildFlags* _childFlags =
        reinterpret_cast< ImGuiChildFlags* >( _callbackArguments[ 2 ] );
    ImGuiWindowFlags* _childWindowFlags =
        reinterpret_cast< ImGuiWindowFlags* >( _callbackArguments[ 3 ] );

    if ( ImGui::BeginChild( *_childId, *_childSize, *_childFlags,
                            *_childWindowFlags ) ) {
        l_returnValue = 1;
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    ImGui$EndChild( void** _callbackArguments ) {
    ImGui::EndChild();

    return ( 0 );
}
