
#ifndef __GIFTI_LABEL_TABLE_SAX_READER_H__
#define __GIFTI_LABEL_TABLE_SAX_READER_H__

/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*LICENSE_END*/

#include <stack>
#include <stdint.h>

#include "AString.h"
#include "SceneObjectDataTypeEnum.h"
#include "XmlSaxParserException.h"
#include "XmlSaxParserHandlerInterface.h"


namespace caret {

    class Scene;
    class SceneClass;
    class SceneObject;
    class SceneObjectArray;
    class XmlAttributes;
    
    /**
     * class for reading a Scene with a SAX Parser
     */
    class SceneSaxReader : public CaretObject, public XmlSaxParserHandlerInterface {
    public:
        SceneSaxReader(const AString& sceneFileName,
                       Scene* scene);
        
        virtual ~SceneSaxReader();
        
        void startElement(const AString& namespaceURI,
                          const AString& localName,
                          const AString& qName,
                          const XmlAttributes& attributes) throw (XmlSaxParserException);
        
        void endElement(const AString& namspaceURI,
                        const AString& localName,
                        const AString& qName) throw (XmlSaxParserException);
        
        void characters(const char* ch) throw (XmlSaxParserException);
        
        void fatalError(const XmlSaxParserException& e) throw (XmlSaxParserException);
        
        void warning(const XmlSaxParserException& e) throw (XmlSaxParserException);
        
        void error(const XmlSaxParserException& e) throw (XmlSaxParserException);
        
        void startDocument() throw (XmlSaxParserException);
        
        void endDocument() throw (XmlSaxParserException);
        
    protected:
        /// file reading states
        enum STATE {
            /// no state
            STATE_NONE,
            /// processing Scene tag
            STATE_SCENE,
            /// processing Scene Name tag
            STATE_SCENE_NAME,
            /// processing Object tag
            STATE_OBJECT,
            /// processing Object Array tag
            STATE_OBJECT_ARRAY,
            /// processing Object Array tag
            STATE_OBJECT_ARRAY_ELEMENT,
            /// processing Object Map tag
            STATE_OBJECT_MAP,
            /// processing Object Map Value tag
            STATE_OBJECT_MAP_VALUE
        };
        
        void processObjectStartTag(const XmlAttributes& attributes) throw (XmlSaxParserException);
        
        void processObjectArrayStartTag(const XmlAttributes& attributes) throw (XmlSaxParserException);
        
        void processObjectMapStartTag(const XmlAttributes& attributes) throw (XmlSaxParserException);
        
        void addChildToParentClass(SceneObject* sceneObject);
        
        /// name of scene file
        AString m_sceneFileName;
        
        /// file reading state
        STATE m_state;
        
        /// the state stack used when reading a file
        std::stack<STATE> m_stateStack;
        
        /// the error message
        AString m_errorMessage;
        
        /// element text
        AString m_elementText;
        
        /// Scene being read
        Scene* m_scene;

        /// Stack containing object being read
        std::stack<SceneObject*> m_objectBeingReadStack;
        
        /** Index of array element being read */
        std::stack<int32_t> m_objectArrayBeingReadElementIndexStack;
        
        /** Key of map value being read */
        std::stack<int32_t> m_objectMapBeingReadValueKeyStack;
    };

} // namespace

#endif // __GIFTI_LABEL_TABLE_SAX_READER_H__

