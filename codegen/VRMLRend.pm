# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id: VRMLRend.pm,v 1.42 2012/07/10 18:40:26 crc_canada Exp $
#
# Name:        VRMLRend.c
# Description:
#              Fills Hash Variables with "C" Code. They are used by VRMLC.pm
#              to write the C functions-source to render different nodes.
#
#              Certain Abbreviation are used, some are substituted in the
#              writing process in get_rendfunc() [VRMLC.pm].
#              Others are "C-#defines".
#              e.g. for #define glTexCoord2f(a,b) glTexCoord2f(a,b) see gen() [VRMLC.pm]
#
# $Log: VRMLRend.pm,v $
# Revision 1.42  2012/07/10 18:40:26  crc_canada
# changing TextureCoordinate handling for shaders; remove "precision" for non-GLES shaders.
#
# Revision 1.41  2012/04/26 16:36:23  crc_canada
# Android changes - preparing for MultiTexturing
#
# Revision 1.40  2011/06/04 19:05:42  crc_canada
# comment out MIDI code
#
# Revision 1.39  2011/05/14 23:00:00  dug9
# dug9 - touch up to get a proximity_GeoProximitySensor virtual table function
#
# Revision 1.38  2010/12/21 20:10:33  crc_canada
# some code changes for Geometry Shaders.
#
# Revision 1.37  2010/12/10 17:17:19  davejoubert
# Add OSC capability to FreeWRL. This update is spread across several files,
# but the two post important changed are in codegen/VRMLNodes.pm and
# src/lib/scenegraph/Component_Networking.c
# Modified Files:
# 	configure.ac codegen/VRMLC.pm codegen/VRMLNodes.pm
# 	codegen/VRMLRend.pm src/lib/Makefile.am
# 	src/lib/Makefile.sources src/lib/main/MainLoop.c
# 	src/lib/main/MainLoop.h
# 	src/lib/scenegraph/Component_Networking.c
# 	src/lib/scenegraph/Component_Networking.h
# 	src/lib/scenegraph/GeneratedCode.c
# 	src/lib/vrml_parser/NodeFields.h src/lib/vrml_parser/Structs.h
# 	src/lib/world_script/jsUtils.c src/libeai/GeneratedCode.c
# Added Files:
# 	src/lib/scenegraph/OSCcallbacks.c src/lib/scenegraph/ringbuf.c
# 	src/lib/scenegraph/ringbuf.h
#
# Revision 1.36  2010/10/12 00:34:12  dug9
# dug9 - codegeneration - added *other() virtual function, and assigned pointpicksensor, pickablegroup and sphere to implement it, put stubs for these other() functions for those that don't implement it.
#
# Revision 1.35  2010/10/02 06:42:25  davejoubert
# Pickables. Scan for DJTRACK_PICKSENSORS
# Modified Files:
#  	codegen/VRMLC.pm codegen/VRMLNodes.pm codegen/VRMLRend.pm
#  	src/lib/main/MainLoop.c src/lib/main/headers.h
#  	src/lib/scenegraph/Component_Geometry3D.c
#  	src/lib/scenegraph/Component_Grouping.c
#  	src/lib/scenegraph/Component_Shape.c
#  	src/lib/scenegraph/GeneratedCode.c
#  	src/lib/vrml_parser/CRoutes.c
#
# Revision 1.34  2010/09/30 16:21:53  crc_canada
# only sort children if required.
#
# Revision 1.33  2010/09/16 18:32:58  crc_canada
# finish removing of "changed_" routines.
#
# Revision 1.32  2010/09/16 15:48:42  crc_canada
# deprecating the "changed_" functions. This does not work well with threading scene graph
#
# Revision 1.31  2010/08/10 21:15:59  crc_canada
# ImageCubeMapTexture - now works - loads png, jpg, etc, NOT DDS yet.
#
# Revision 1.30  2010/07/29 14:32:27  crc_canada
# OrthoViewpoint and ViewpointGroup now working
#
# Revision 1.29  2010/07/28 00:14:52  crc_canada
# initial OrthoViewpoint code in place. Not activated; just compile framework.
#
# Revision 1.28  2010/06/29 22:13:36  davejoubert
# Implement PickableGroup
# Modified Files:
# 	codegen/VRMLC.pm codegen/VRMLNodes.pm codegen/VRMLRend.pm
# 	src/lib/internal.h src/lib/input/SensInterps.h
# 	src/lib/scenegraph/Component_Grouping.c
# 	src/lib/scenegraph/GeneratedCode.c
# 	src/lib/vrml_parser/NodeFields.h src/lib/vrml_parser/Structs.h
# 	src/lib/world_script/fieldSet.c src/libeai/GeneratedCode.c
#
# Revision 1.27  2010/03/26 18:16:29  crc_canada
# DIS headers put in VRMLNodes.pm
#
# Revision 1.26  2009/12/28 15:57:46  crc_canada
# TextureProperties node now active.
#
# Revision 1.25  2009/11/06 22:29:42  crc_canada
# Shader Routing, and Metadata MARK_EVENT working.
#
# Revision 1.24  2009/10/21 19:18:30  crc_canada
# Working on XML parsing and Material rendering. compile_TwoSidedMaterials added; XML parsing modified to clean up memory; and to better handle PROTO expansions. Still have a bit more work to do on SF/MFNode Proto/Script fields.
#
# Revision 1.23  2009/10/02 21:34:52  crc_canada
# start gathering appearance properties together for eventual movement to OpenGL-ES and OpenGL-3.0
#
# Revision 1.22  2009/09/18 20:20:29  crc_canada
# Starting ExternProtoDeclare for XML parsing.
#
# Revision 1.21  2009/08/27 18:34:32  crc_canada
# More XML coded PROTO routing.
#
# Revision 1.20  2009/07/24 18:09:19  crc_canada
# Geometry2D extent calculations now performed for all 2D shapes.
#
# Revision 1.19  2009/07/14 15:36:01  uid31638
# More Geospatial code changes; GeoLOD in particular has been worked on.
#
# Revision 1.18  2009/07/02 15:45:36  crc_canada
# setExtent for PointSet, LineSet and IndexedLineSet
#
# Revision 1.17  2009/06/26 14:54:15  crc_canada
# GeoElevationGrid - no longer has ElevationGrid sub child, and triangle winding
#
# Revision 1.16  2009/06/22 19:40:41  crc_canada
# more VRML1 work.
#
# Revision 1.15  2009/06/19 16:21:44  crc_canada
# VRML1 work.
#
# Revision 1.14  2009/06/18 20:27:02  crc_canada
# VRML1 code.
#
# Revision 1.13  2009/06/17 21:11:00  crc_canada
# more VRML1 code.
#
# Revision 1.12  2009/06/17 18:50:42  crc_canada
# More VRML1 code entered.
#
# Revision 1.11  2009/06/17 15:05:24  crc_canada
# VRML1 nodes added to build process.
#
# Revision 1.10  2009/06/05 20:29:32  crc_canada
# verifying fields of nodes against spec.
#
# Revision 1.9  2009/05/22 16:18:40  crc_canada
# more XML formatted code script/shader programming.
#
# Revision 1.8  2009/05/21 20:30:08  crc_canada
# XML parser - scripts and shaders now using common routing and maintenance routines.
#
# Revision 1.7  2009/05/12 19:53:14  crc_canada
# Confirm current support levels, and verify that Components and Profiles are checked properly.
#
# Revision 1.6  2009/05/11 21:11:58  crc_canada
# local/global lighting rules applied to SpotLight, DirectionalLight and PointLight.
#
# Revision 1.5  2009/04/02 18:48:28  crc_canada
# PROTO Routing for MFNodes.
#
# Revision 1.4  2009/03/10 21:00:34  crc_canada
# checking in some ongoing PROTO support work in the Classic parser.
#
# Revision 1.3  2009/03/09 21:32:30  crc_canada
# initial handling of new PROTO parameter methodology
#
# Revision 1.2  2009/03/06 18:50:31  istakenv
# fixed metadata variable names
#
# Revision 1.1  2009/03/05 21:33:39  istakenv
# Added code-generator perl scripts to new freewrl tree.  Initial commit, still need to patch them to make them work.
#
# Revision 1.232  2009/03/03 16:59:14  crc_canada
# Metadata fields now verified to be in every X3D node.
#
# Revision 1.231  2009/01/29 16:01:21  crc_canada
# more node definitions.
#
# Revision 1.230  2008/10/29 21:07:05  crc_canada
# Work on fillProperties and TwoSidedMaterial.
#
# Revision 1.229  2008/10/29 18:32:07  crc_canada
# Add code to confirm Profiles and Components.
#
# Revision 1.228  2008/10/23 19:18:53  crc_canada
# CubeMap texturing - start.
#
# Revision 1.227  2008/10/02 15:38:42  crc_canada
# Shader support started; Geospatial eventOut verification.
#
# Revision 1.226  2008/09/24 19:23:01  crc_canada
# GeoTouchSensor work.
#
# Revision 1.225  2008/09/23 16:45:02  crc_canada
# initial GeoTransform code added.
#
# Revision 1.224  2008/09/22 16:06:48  crc_canada
# all fieldtypes now defined in freewrl code; some not parsed yet, though, as there are no supported
# nodes that use them.
#
# Revision 1.223  2008/09/05 17:46:49  crc_canada
# reduce warnings counts when compiled with warnings=all
#
# Revision 1.222  2008/08/18 14:45:38  crc_canada
# Billboard node Scene Graph changes.
#
# Revision 1.221  2008/08/14 05:02:32  crc_canada
# Bindable threading issues, continued; EXAMINE mode default rotation distance, continued; LOD improvements.
#
# Revision 1.220  2008/08/04 19:14:36  crc_canada
# August 4 GeoLOD  changes
#
# Revision 1.219  2008/07/30 18:08:34  crc_canada
# GeoLOD, July 30 changes.
#
# Revision 1.218  2008/06/17 19:00:27  crc_canada
# Geospatial work - June 17 2008
#
# Revision 1.217  2008/06/13 13:50:49  crc_canada
# Geospatial, SF/MFVec3d support.
#
# Revision 1.216  2008/05/07 15:22:41  crc_canada
# input function modified to better handle files without clear end of line on last line.
#
# Revision 1.215  2008/03/31 20:10:17  crc_canada
# Review texture transparency, use node table to update scenegraph to allow for
# node updating.
#
# Revision 1.214  2007/12/13 20:12:52  crc_canada
# KeySensor and StringSensor
#
# Revision 1.213  2007/12/12 23:24:58  crc_canada
# X3DParser work
#
# Revision 1.212  2007/12/10 19:13:53  crc_canada
# Add parsing for x3dv COMPONENT, EXPORT, IMPORT, META, PROFILE
#
# Revision 1.211  2007/12/08 13:38:17  crc_canada
# first changes for x3dv handling of META, COMPONENT, etc. taga.
#
# Revision 1.210  2007/12/06 21:50:57  crc_canada
# Javascript X3D initializers
#
# Revision 1.209  2007/11/06 20:25:28  crc_canada
# Lighting revisited - pointlights and spotlights should all now work ok
#
# Revision 1.208  2007/08/23 14:01:22  crc_canada
# Initial AudioControl work
#
# Revision 1.207  2007/03/12 20:54:00  crc_canada
# MidiKey started.
#
# Revision 1.206  2007/02/28 20:34:50  crc_canada
# More MIDI work - channelPresent works!
#
# Revision 1.205  2007/01/17 21:29:28  crc_canada
# more X3D XML parsing work.
#
# Revision 1.204  2007/01/12 17:55:27  crc_canada
# more 1.18.11 changes
#
# Revision 1.203  2007/01/11 21:09:21  crc_canada
# new files for X3D parsing
#
# Revision 1.201  2006/11/14 20:16:39  crc_canada
# ReWire work.
#
# Revision 1.200  2006/07/10 14:24:11  crc_canada
# add keywords for PROTO interface fields.
#
# Revision 1.199  2006/05/31 14:52:28  crc_canada
# more changes to code for SAI.
#
# Revision 1.198  2006/05/23 16:15:10  crc_canada
# remove print statements, add more defines for a VRML C parser.
#
# Revision 1.197  2006/05/15 14:05:59  crc_canada
# Various fixes; CVS was down for a week. Multithreading for shape compile
# is the main one.
#
# Revision 1.196  2006/03/01 15:16:57  crc_canada
# Changed include file methodology and some Frustum work.
#
# Revision 1.195  2006/02/28 16:19:42  crc_canada
# BoundingBox
#
# Revision 1.194  2006/01/06 22:05:15  crc_canada
# VisibilitySensor
#
# Revision 1.193  2006/01/03 23:01:22  crc_canada
# EXTERNPROTO and Group sorting bugs fixed.
#
# Revision 1.192  2005/12/21 18:16:40  crc_canada
# Rework Generation code.
#
# Revision 1.191  2005/12/19 21:25:08  crc_canada
# HAnim start
#
# Revision 1.190  2005/12/16 18:07:11  crc_canada
# rearrange perl generation
#
# Revision 1.189  2005/12/16 13:49:23  crc_canada
# updating generation functions.
#
# Revision 1.188  2005/12/15 20:42:01  crc_canada
# CoordinateInterpolator2D PositionInterpolator2D
#
# Revision 1.187  2005/12/15 19:57:58  crc_canada
# Geometry2D nodes complete.
#
# Revision 1.186  2005/12/14 13:51:32  crc_canada
# More Geometry2D nodes.
#
# Revision 1.185  2005/12/13 17:00:29  crc_canada
# Arc2D work.
#.....

# DJTRACK_PICKSENSORS See PointPickSensor
# See WANT_OSC
# used for the X3D Parser only. Return type of node.
%defaultContainerType = (
	ContourPolyLine2D 	=>geometry,
	NurbsTrimmedSurface	=>geometry,
	PointPickSensor		=>children,
	OSC_Sensor		=>children,

	Arc2D			=>geometry,
	ArcClose2D		=>geometry,
	Circle2D		=>geometry,
	Disk2D			=>geometry,
	Polyline2D		=>geometry,
	Polypoint2D		=>geometry,
	Rectangle2D		=>geometry,
	TriangleSet2D		=>geometry,
	
	Anchor 			=>children,
	Appearance 		=>appearance,
	AudioClip 		=>source,
	Background 		=>children,
	Billboard 		=>children,
	Box 			=>geometry,
	ClipPlane 		=>children,
	Collision 		=>children,
	Color 			=>color,
	ColorInterpolator 	=>children,
	ColorRGBA 		=>color,
	Cone 			=>geometry,
	Contour2D 		=>geometry,
	Coordinate 		=>coord,
	FogCoordinate 		=>coord,
	CoordinateDeformer 	=>children,
	CoordinateInterpolator 	=>children,
	CoordinateInterpolator2D 	=>children,
	Cylinder 		=>geometry,
	CylinderSensor 		=>children,
	DirectionalLight 	=>children,
	ElevationGrid 		=>geometry,
	Extrusion 		=>geometry,
	FillProperties		=>fillProperties,
	Fog 			=>children,
	LocalFog 		=>children,
	FontStyle 		=>fontStyle,
	GeoCoordinate 		=>coord,
	GeoElevationGrid 	=>geometry,
	GeoLocation 		=>children,
	GeoLOD 			=>children,
	GeoMetadata		=>children,
	GeoOrigin 		=>geoOrigin,
	GeoPositionInterpolator	=>children,
	GeoProximitySensor 	=>children,
	GeoTouchSensor		=>children,
	GeoTransform		=>children,
	GeoViewpoint 		=>children,
	Group 			=>children,
	ViewpointGroup		=>children,
	HAnimDisplacer		=>children,
	HAnimHumanoid		=>children,
	HAnimJoint		=>joints,
	HAnimSegment		=>segments,
	HAnimSite		=>sites,
	ImageTexture 		=>texture,
	ImageCubeMapTexture 	=>texture,
	GeneratedCubeMapTexture	=>texture, 
	ComposedCubeMapTexture	=>texture,
	IndexedFaceSet 		=>geometry,
	IndexedLineSet 		=>geometry,
	IndexedTriangleFanSet 	=>geometry,
	IndexedTriangleSet 	=>geometry,
	IndexedTriangleStripSet	=>geometry,
	Inline 			=>children,
	KeySensor		=>children,
	LineSet 		=>geometry,
	LineProperties		=>lineProperties,
	LoadSensor		=>children,
	LOD 			=>children,
	Material 		=>material,
	TwoSidedMaterial	=>material,
	MultiTexture		=>texture,
	MultiTextureCoordinate  =>texCoord,
	MultiTextureTransform	=>textureTransform,
	MovieTexture 		=>texture,
	NavigationInfo 		=>children,
	Normal 			=>normal,
	NormalInterpolator 	=>children,
	NurbsCurve2D 		=>geometry,
	NurbsCurve 		=>geometry,
	NurbsGroup 		=>children,
	NurbsPositionInterpolator=>children,
	NurbsSurface 		=>children,
	NurbsTextureSurface 	=>children,
	OrientationInterpolator	=>children,
	PickableGroup 		=>children,
	PixelTexture 		=>texture,
	PlaneSensor 		=>children,
	PointLight 		=>children,
	PointSet 		=>geometry,
	PositionInterpolator 	=>children,
	PositionInterpolator2D 	=>children,
	ProximitySensor 	=>children,
	ScalarInterpolator 	=>children,
	Scene 			=>children,
	Script 			=>children,
	Shape 			=>children,
	Sound 			=>children,
	Sphere 			=>geometry,
	SphereSensor 		=>children,
	SpotLight 		=>children,
	StaticGroup		=>children,
	StringSensor		=>children,
	Switch 			=>children,
	Text 			=>geometry,
	TextureBackground 	=>children,
	TextureCoordinate 	=>texCoord,
	TextureCoordinateGenerator  =>texCoord,
	TextureTransform 	=>textureTransform,
	TextureProperties	=>children,
	TimeSensor 		=>children,
	TouchSensor 		=>children,
	Transform 		=>children,
	TriangleFanSet 		=>geometry,
	TriangleSet 		=>geometry,
	TriangleStripSet 	=>geometry,
	TrimmedSurface 		=>children,
	Viewpoint 		=>children,
	OrthoViewpoint 		=>children,
	VisibilitySensor 	=>children,
	WorldInfo 		=>children,

	BooleanFilter		=>children,
	BooleanSequencer	=>children,
	BooleanToggle		=>children,
	BooleanTrigger		=>children,
	IntegerSequencer	=>children,
	IntegerTrigger		=>children,
	TimeTrigger		=>children,

	ComposedShader		=>shaders,
	FloatVertexAttribute	=>children,
	Matrix3VertexAttribute	=>children,
	Matrix4VertexAttribute	=>children,
	PackagedShader		=>material,
	ProgramShader		=>programs,
	ShaderPart		=>parts,
	ShaderProgram		=>material,

	MetadataSet		=>metadata,
	MetadataInteger		=>metadata,
	MetadataDouble		=>metadata,
	MetadataFloat		=>metadata,
	MetadataString		=>metadata,

	
	MetadataSFFloat		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFFloat		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFRotation	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFRotation	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec3f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec3f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFBool		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFBool		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFInt32		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFInt32		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFNode		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFNode		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFColor		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFColor		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFColorRGBA	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFColorRGBA	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFTime		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFTime		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFString	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFString	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec2f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec2f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFImage		=>FreeWRL_PROTOInterfaceNodes,
	MetadataFreeWRLPTR	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec3d		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec3d		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFDouble	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFDouble	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFMatrix3f	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFMatrix3f	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFMatrix3d	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFMatrix3d	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFMatrix4f	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFMatrix4f	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFMatrix4d	=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFMatrix4d	=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec2d		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec2d		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec4f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec4f		=>FreeWRL_PROTOInterfaceNodes,
	MetadataSFVec4d		=>FreeWRL_PROTOInterfaceNodes,
	MetadataMFVec4d		=>FreeWRL_PROTOInterfaceNodes,

	EaseInEaseOut 	=>children,
	SplinePositionInterpolator 	=>children,
	SplinePositionInterpolator2D 	=>children,
	SplineScalarInterpolator 	=>children,
	SquadOrientationInterpolator 	=>children,



	VRML1_AsciiText		=>children,
	VRML1_Cone		=>children,
	VRML1_Coordinate3	=>children,
	VRML1_Cube		=>children,
	VRML1_Cylinder		=>children,
	VRML1_DirectionalLight	=>children,
	VRML1_FontStyle		=>children,
	VRML1_IndexedFaceSet	=>children,
	VRML1_IndexedLineSet	=>children,
	VRML1_Info		=>children,
	VRML1_LOD		=>children,
	VRML1_Material		=>children,
	VRML1_MaterialBinding	=>children,
	VRML1_MatrixTransform	=>children,
	VRML1_Normal		=>children,
	VRML1_NormalBinding	=>children,
	VRML1_OrthographicCamera=>children,
	VRML1_PerspectiveCamera	=>children,
	VRML1_PointLight	=>children,
	VRML1_PointSet		=>children,
	VRML1_Rotation		=>children,
	VRML1_Scale		=>children,
	VRML1_Separator		=>children,
	VRML1_ShapeHints	=>children,
	VRML1_Sphere		=>children,
	VRML1_SpotLight		=>children,
	VRML1_Switch		=>children,
	VRML1_Texture2		=>children,
	VRML1_Texture2Transform	=>children,
	VRML1_TextureCoordinate2=>children,
	VRML1_Transform		=>children,
	VRML1_Translation	=>children,
	VRML1_WWWAnchor		=>children,
	VRML1_WWWInline		=>children,

	DISEntityManager	=>children,
	DISEntityTypeMapping	=>children,
	EspduTransform		=>children,
	ReceiverPdu		=>children,
	SignalPdu		=>children,
	TransmitterPdu		=>children,


);

#######################################################################
#######################################################################
#######################################################################
#
# Rend --
#  actually render the node
#
#

# Rend = real rendering - rend_geom is true; this is for things that
#	actually affect triangles/lines on the screen.
#
# All of these will have a render_xxx name associated with them.

%RendC = map {($_=>1)} qw/
	NavigationInfo
	Fog
	Background
	TextureBackground
	Box 
	Cylinder 
	Cone 
	Sphere 
	IndexedFaceSet 
	Extrusion 
	ElevationGrid 
	Arc2D 
	ArcClose2D 
	Circle2D 
	Disk2D 
	Polyline2D 
	Polypoint2D 
	Rectangle2D 
	TriangleSet2D 
	IndexedTriangleFanSet 
	IndexedTriangleSet 
	IndexedTriangleStripSet 
	TriangleFanSet 
	TriangleStripSet 
	TriangleSet 
	LineSet 
	IndexedLineSet 
	PointSet 
	GeoElevationGrid 
	LoadSensor 
	TextureCoordinateGenerator 
	Text 
	LineProperties 
	FillProperties 
	Material 
	TwoSidedMaterial 
	ProgramShader
	PackagedShader
	ComposedShader
	PixelTexture 
	ImageTexture 
	MultiTexture 
	MovieTexture 
	ComposedCubeMapTexture
	GeneratedCubeMapTexture
	ImageCubeMapTexture
	Sound 
	AudioClip 
	DirectionalLight 
	SpotLight
	PointLight
	HAnimHumanoid
	HAnimJoint

	VRML1_AsciiText
	VRML1_Cone
	VRML1_Cube
	VRML1_Cylinder
	VRML1_IndexedFaceSet
	VRML1_IndexedLineSet
	VRML1_PointSet
	VRML1_Sphere
	VRML1_Scale
	VRML1_Translation
	VRML1_Transform
	VRML1_Material
	VRML1_Rotation
	VRML1_DirectionalLight
	VRML1_SpotLight
	VRML1_PointLight
	VRML1_Coordinate3
	VRML1_FontStyle
	VRML1_MaterialBinding
	VRML1_Normal
	VRML1_NormalBinding
	VRML1_Texture2
	VRML1_Texture2Transform
	VRML1_TextureCoordinate2
	VRML1_ShapeHints
	VRML1_MatrixTransform
/;

#######################################################################
#######################################################################
#######################################################################

#
# GenPolyRep
#  code for generating internal polygonal representations
#  of some nodes (ElevationGrid, Text, Extrusion and IndexedFaceSet)
#
# 

%GenPolyRepC = map {($_=>1)} qw/
	ElevationGrid
	Extrusion
	IndexedFaceSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	Text
	GeoElevationGrid
	VRML1_IndexedFaceSet
/;

#######################################################################
#######################################################################
#######################################################################
#
# Prep --
#  Prepare for rendering a node - e.g. for transforms, do the transform
#  but not the children.

%PrepC = map {($_=>1)} qw/
	HAnimJoint
	HAnimSite
	Viewpoint
	OrthoViewpoint
	Transform
	Billboard
	Group
	PickableGroup
	PointLight
	SpotLight
	DirectionalLight
	GeoLocation
	GeoViewpoint
	GeoTransform
	VRML1_Separator
/;

#######################################################################
#######################################################################
#######################################################################
#
# Fin --
#  Finish the rendering i.e. restore matrices and whatever to the
#  original state.
#
#

%FinC = map {($_=>1)} qw/
	GeoLocation
	Transform
	Billboard
	HAnimSite
	HAnimJoint
	GeoTransform
	VRML1_Separator
/;


#######################################################################
#######################################################################
#######################################################################
#
# Child --
#  Render the actual children of the node.
#
#

# Render children (real child nodes, not e.g. appearance/geometry)
%ChildC = map {($_=>1)} qw/
	HAnimHumanoid
	HAnimJoint
	HAnimSegment
	HAnimSite
	Group
	ViewpointGroup
	StaticGroup
	PickableGroup
	Billboard
	Transform
	Anchor
	GeoLocation
	GeoTransform
	Inline
	Switch
	GeoLOD
	LOD
	Collision
	Appearance
	Shape
	VisibilitySensor
	VRML1_Separator
/;


#######################################################################
#######################################################################
#######################################################################
#
# Compile --
#
%CompileC = map {($_=>1)} qw/
	Shape
	ImageCubeMap
	Transform
	Group
	ViewpointGroup
	Material
	TwoSidedMaterial
	IndexedLineSet
	LineSet
	PointSet
	Arc2D
	ArcClose2D
	Circle2D
	Disk2D
	TriangleSet2D
	Rectangle2D
	Polyline2D
	Polypoint2D
	Box
	Cone
	Cylinder
	Sphere
	GeoLocation
	GeoCoordinate
	GeoElevationGrid
	GeoLocation
	GeoLOD
	GeoMetadata
	GeoOrigin
	GeoPositionInterpolator
	GeoTouchSensor
	GeoViewpoint	
	GeoProximitySensor
	GeoTransform
	ComposedShader
	ProgramShader
	PackagedShader
	MetadataMFFloat
	MetadataMFRotation
	MetadataMFVec3f
	MetadataMFBool
	MetadataMFInt32
	MetadataMFNode
	MetadataMFColor
	MetadataMFColorRGBA
	MetadataMFTime
	MetadataMFString
	MetadataMFVec2f
	MetadataMFVec3d
	MetadataMFDouble
	MetadataMFMatrix3f
	MetadataMFMatrix3d
	MetadataMFMatrix4f
	MetadataMFMatrix4d
	MetadataMFVec2d
	MetadataMFVec4f
	MetadataMFVec4d
	MetadataSFFloat
	MetadataSFRotation
	MetadataSFVec3f
	MetadataSFBool
	MetadataSFInt32
	MetadataSFNode
	MetadataSFColor
	MetadataSFColorRGBA
	MetadataSFTime
	MetadataSFString
	MetadataSFVec2f
	MetadataSFImage
	MetadataSFVec3d
	MetadataSFDouble
	MetadataSFMatrix3f
	MetadataSFMatrix3d
	MetadataSFMatrix4f
	MetadataSFMatrix4d
	MetadataSFVec2d
	MetadataSFVec4f
	MetadataSFVec4d
	MetadataSet
	MetadataInteger	
	MetadataDouble
	MetadataFloat
	MetadataString
/;


#######################################################################
#
# ProximityC = following code is run to let proximity sensors send their
# events. This is done in the rendering pass, because the position of
# of the object relative to the viewer is available via the
# modelview transformation matrix.
#

%ProximityC = map {($_=>1)} qw/
	ProximitySensor
	LOD
	Billboard
	GeoProximitySensor
/;

#######################################################################
#
# OtherC = following code is run for miscalaneous tasks depending 
# on what functions you define here and what VF flags you define and
# check against in render_node() and/or switch on in your Other() 
#

%OtherC = map {($_=>1)} qw/
	PointPickSensor
	PickableGroup
	Sphere
/;


#######################################################################
#
# CollisionC = following code is run to do collision detection
#
# In collision nodes:
#    if enabled:
#       if no proxy:
#           passes rendering to its children
#       else (proxy)
#           passes rendering to its proxy
#    else
#       does nothing.
#
# In normal nodes:
#    uses gl modelview matrix to determine distance from viewer and
# angle from viewer. ...
#
#
#	       /* the shape of the avatar is a cylinder */
#	       /*                                           */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           |--|                            */
#	       /*           | width                         */
#	       /*        ---|---       -                    */
#	       /*        |     |       |                    */
#	       /*    ----|() ()| - --- | ---- y=0           */
#	       /*        |  \  | |     |                    */
#	       /*     -  | \ / | |head | height             */
#	       /*    step|     | |     |                    */
#	       /*     -  |--|--| -     -                    */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           x,z=0                           */


%CollisionC = map {($_=>1)} qw/
	Disk2D
	Rectangle2D
	TriangleSet2D
	Sphere
	Box
	Cone
	Cylinder
	ElevationGrid
	IndexedFaceSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	Extrusion
	Text
	GeoElevationGrid
	VRML1_IndexedFaceSet
/;

#######################################################################
#######################################################################
#######################################################################
#
# RendRay --
#  code for checking whether a ray (defined by mouse pointer)
#  intersects with the geometry of the primitive.
#
#

# Y axis rotation around an unit vector:
# alpha = angle between Y and vec, theta = rotation angle
#  1. in X plane ->
#   Y = Y - sin(alpha) * (1-cos(theta))
#   X = sin(alpha) * sin(theta)
#
#
# How to find out the orientation from two vectors (we are allowed
# to assume no negative scales)
#  1. Y -> Y' -> around any vector on the plane midway between the
#                two vectors
#     Z -> Z' -> around any vector ""
#
# -> intersection.
#
# The plane is the midway normal between the two vectors
# (if the two vectors are the same, it is the vector).


# found in the C code:
# Distance to zero as function of ratio is
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.y + r t_r2.y)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius
# Therefore,
# radius ** 2 == ... ** 2
# and
# radius ** 2 =
# 	(1-r)**2 * (t_r1.x**2 + t_r1.y**2 + t_r1.z**2) +
#       2*(r*(1-r)) * (t_r1.x*t_r2.x + t_r1.y*t_r2.y + t_r1.z*t_r2.z) +
#       r**2 (t_r2.x**2 ...)
# Let's name tr1sq, tr2sq, tr1tr2 and then we have
# radius ** 2 =  (1-r)**2 * tr1sq + 2 * r * (1-r) tr1tr2 + r**2 tr2sq
# = (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + tr1sq
#
# I.e.
#
# (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) +
#	(tr1sq - radius**2) == 0
#
# I.e. second degree eq. a r**2 + b r + c == 0 where
#  a = tr1sq - 2*tr1tr2 + tr2sq
#  b = 2*(tr1tr2 - tr1sq)
#  c = (tr1sq-radius**2)
#
#
# Cylinder: first test the caps, then against infinite cylinder.

# For cone, this is most difficult. We have
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius*( -( (1-r)t_r1.y + r t_r2.y )/(2*h)+0.5)
# == radius * ( -( r*(t_r2.y - t_r1.y) + t_r1.y )/(2*h)+0.5)
# == radius * ( -r*(t_r2.y-t_r1.y)/(2*h) + 0.5 - t_r1.y/(2*h))

#
# Other side: r*r*(

%RendRayC = map {($_=>1)} qw/
	Box
	Sphere
	Cylinder
	Cone
	GeoElevationGrid
	ElevationGrid
	Text
	Extrusion
	IndexedFaceSet
	IndexedTriangleSet
	IndexedTriangleFanSet
	IndexedTriangleStripSet
	TriangleSet
	TriangleFanSet
	TriangleStripSet
	VRML1_IndexedFaceSet
/;

#######################################################################
#######################################################################
#######################################################################
#
# VRML1_ Keywords
%VRML1_C = map {($_=>1)} qw/
	VRML1_AsciiText
	VRML1_Cone
	VRML1_Cube
	VRML1_Cylinder
	VRML1_IndexedFaceSet
	VRML1_IndexedLineSet
	VRML1_PointSet
	VRML1_Sphere
	VRML1_Coordinate3
	VRML1_FontStyle
	VRML1_Info
	VRML1_Material
	VRML1_MaterialBinding
	VRML1_Normal
	VRML1_NormalBinding
	VRML1_Texture2
	VRML1_Texture2Transform
	VRML1_TextureCoordinate2
	VRML1_ShapeHints
	VRML1_MatrixTransform
	VRML1_Rotation
	VRML1_Scale
	VRML1_Transform
	VRML1_Translation
	VRML1_Separator
	VRML1_Switch
	VRML1_WWWAnchor
	VRML1_LOD
	VRML1_OrthographicCamera
	VRML1_PerspectiveCamera
	VRML1_DirectionalLight
	VRML1_PointLight
	VRML1_SpotLight
	VRML1_WWWInline
/;

#######################################################################
#######################################################################
#######################################################################
#
# Keywords
# a listing of keywords for use in the C VRML parser.
#
# 

%KeywordC = map {($_=>1)} qw/
	COMPONENT
	DEF
	EXPORT
	EXTERNPROTO
	FALSE
	IMPORT
	IS
	META
	NULL
	PROFILE
	PROTO
	ROUTE
	TO
	TRUE
	USE
	inputOnly
	outputOnly
	inputOutput
	initializeOnly
	exposedField
	field
	eventIn
	eventOut
/;


#######################################################################
#
# Components 
# a listing of Components for use in the C VRML parser.
#
# 

%ComponentC = map {($_=>1)} qw/
	CADGeometry
	Core
	CubeMapTexturing
	DIS
	EnvironmentalEffects
	EnvironmentalSensor
	EventUtilities
	Followers
	Geometry2D
	Geometry3D
	Geospatial
	Grouping
	H-Anim
	Interpolation
	KeyDeviceSensor
	Layering
	Layout
	Lighting
	Navigation
	Networking
	NURBS
	ParticleSystems
	PickingSensor
	PointDeviceSensor
	Shaders
	Rendering
	RigidBodyPhysics
	Scripting
	Shape
	Sound
	Text
	Texturing
	Texturing3D
	Time
/;


#######################################################################
#
# Profiles 
# a listing of Profiles for use in the C VRML parser.
#
# 

%ProfileC = map {($_=>1)} qw/
	CADInterchange
	Core
	Full
	Immersive
	Interactive
	Interchange
	MPEG-4
/;

%VRML1ModifierC = map {($_=>1)} qw/
	SIDES 
	BOTTOM  
	ALL 
	TOP 
	LEFT 
	CENTER 
	RIGHT 
	SERIF
	SANS 
	TYPEWRITER 
	NONE 
	BOLD 
	ITALIC 
	DEFAULT 
	OVERALL 
	PER_PART 
	PER_PART_INDEXED
	PER_FACE 
	PER_FACE_INDEXED 
	PER_VERTEX 
	PER_VERTEX_INDEXED 
	ON 
	OFF 
	AUTO 
	UNKNOWN_SHAPE_TYPE
	CLOCKWISE 
	COUNTERCLOCKWISE 
	SOLID 
	UNKNOWN_ORDERING 
	UNKNOWN_FACE_TYPE 
	CONVEX 
	REPEAT 
	CLAMP
	NONE 
	POINT
/;

#######################################################################
#######################################################################
#######################################################################

#
# GEOSpatialKeywords
# a listing of Geospatial Elipsoid keywords.
#
# 

%GEOSpatialKeywordC = map {($_=>1)} qw/
	AA
	AM
	AN
	BN
	BR
	CC
	CD
	EA
	EB
	EC
	ED
	EE
	EF
	FA
	GC
	GCC
	GCC
	GD
	GDC
	GDC
	HE
	HO
	ID
	IN
	KA
	RF
	SA
	UTM
	WD
	WE
	WGS84
	coordinateSystem
	copyright
	dataFormat
	dataUrl
	date
	description
	ellipsoid
	extent
	horizontalDatum
	metadataFormat
	originator
	resolution
	title
	verticalDatum
/;

#######################################################################
#######################################################################
#######################################################################

#
# PROTOKeywords
# a listing of PROTO define keywords for use in the C VRML parser.
#
# 

%PROTOKeywordC = map {($_=>1)} qw/
	exposedField
	field
	eventIn
	eventOut
	inputOnly
	outputOnly
	inputOutput
	initializeOnly
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Boundary Keywords
# 

%TextureBoundaryC = map {($_=>1)} qw/
	CLAMP
	CLAMP_TO_EDGE
	CLAMP_TO_BOUNDARY
	MIRRORED_REPEAT
	REPEAT
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture MAgnifiation Keywords
# 

%TextureMagnificationC = map {($_=>1)} qw/
	AVG_PIXEL
	DEFAULT
	FASTEST
	NEAREST_PIXEL
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Minification Keywords
# 

%TextureMinificationC = map {($_=>1)} qw/
	AVG_PIXEL
	AVG_PIXEL_AVG_MIPMAP
	AVG_PIXEL_NEAREST_MIPMAP
	DEFAULT
	FASTEST
	NEAREST_PIXEL
	NEAREST_PIXEL_AVG_MIPMAP
	NEAREST_PIXEL_NEAREST_MIPMAP
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Compression Keywords
# 

%TextureCompressionC = map {($_=>1)} qw/
	DEFAULT
	FASTEST
	HIGH
	LOW
	MEDIUM
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

%MultiTextureSourceC = map {($_=>1)} qw/
	DIFFUSE
	SPECULAR
	FACTOR
/;
%MultiTextureFunctionC = map {($_=>1)} qw/
	COMPLEMENT
	ALPHAREPLICATE
/;


%MultiTextureModeC = map {($_=>1)} qw/
	MODULATE2X
	MODULATE4X
	ADDSMOOTH
	BLENDDIFFUSEALPHA
	BLENDCURRENTALPHA
	MODULATEALPHA_ADDCOLOR
	MODULATEINVALPHA_ADDCOLOR
	MODULATEINVCOLOR_ADDALPHA
	SELECTARG1
	SELECTARG2
	DOTPRODUCT3
	MODULATE
	REPLACE
	SUBTRACT
	ADDSIGNED2X
	ADDSIGNED
	ADD
	OFF
/;

#
# X3DSPECIAL Keywords
# a listing of control keywords for use in the XML parser.
#
# 

%X3DSpecialC = map {($_=>1)} qw/
	Scene
	Header
	head
	meta
	ExternProtoDeclare
	ProtoDeclare
	ProtoInterface
	ProtoInstance
	ProtoBody
	ROUTE
	IS
	connect
	X3D
	field
	fieldValue
	component
	import
	export
/;
1;
