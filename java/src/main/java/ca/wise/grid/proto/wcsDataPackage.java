// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: wcsData.proto

package ca.wise.grid.proto;

public final class wcsDataPackage {
  private wcsDataPackage() {}
  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistryLite registry) {
  }

  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistry registry) {
    registerAllExtensions(
        (com.google.protobuf.ExtensionRegistryLite) registry);
  }
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_wcsData_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_wcsData_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_wcsData_locationFile_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_wcsData_locationFile_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_wcsData_binaryData_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_wcsData_binaryData_fieldAccessorTable;

  public static com.google.protobuf.Descriptors.FileDescriptor
      getDescriptor() {
    return descriptor;
  }
  private static  com.google.protobuf.Descriptors.FileDescriptor
      descriptor;
  static {
    java.lang.String[] descriptorData = {
      "\n\rwcsData.proto\022\016WISE.GridProto\032\036google/" +
      "protobuf/wrappers.proto\"\377\002\n\007wcsData\022\017\n\007v" +
      "ersion\030\001 \001(\005\0224\n\004file\030\002 \001(\0132$.WISE.GridPr" +
      "oto.wcsData.locationFileH\000\022\r\n\005xSize\030\004 \001(" +
      "\r\022\r\n\005ySize\030\005 \001(\r\0222\n\006binary\030\006 \001(\0132\".WISE." +
      "GridProto.wcsData.binaryData\032k\n\014location" +
      "File\022\017\n\007version\030\001 \001(\005\022\020\n\010filename\030\002 \001(\t\022" +
      "8\n\022projectionFilename\030\003 \001(\0132\034.google.pro" +
      "tobuf.StringValue\032[\n\nbinaryData\022\014\n\004data\030" +
      "\001 \001(\014\022\021\n\tdataValid\030\002 \001(\014\022,\n\010isZipped\030\003 \001" +
      "(\0132\032.google.protobuf.BoolValueB\006\n\004dataJ\004" +
      "\010\003\020\004R\003gisB7\n\022ca.wise.grid.protoB\016wcsData" +
      "PackageP\001\252\002\016WISE.GridProtob\006proto3"
    };
    descriptor = com.google.protobuf.Descriptors.FileDescriptor
      .internalBuildGeneratedFileFrom(descriptorData,
        new com.google.protobuf.Descriptors.FileDescriptor[] {
          com.google.protobuf.WrappersProto.getDescriptor(),
        });
    internal_static_WISE_GridProto_wcsData_descriptor =
      getDescriptor().getMessageTypes().get(0);
    internal_static_WISE_GridProto_wcsData_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_wcsData_descriptor,
        new java.lang.String[] { "Version", "File", "XSize", "YSize", "Binary", "Data", });
    internal_static_WISE_GridProto_wcsData_locationFile_descriptor =
      internal_static_WISE_GridProto_wcsData_descriptor.getNestedTypes().get(0);
    internal_static_WISE_GridProto_wcsData_locationFile_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_wcsData_locationFile_descriptor,
        new java.lang.String[] { "Version", "Filename", "ProjectionFilename", });
    internal_static_WISE_GridProto_wcsData_binaryData_descriptor =
      internal_static_WISE_GridProto_wcsData_descriptor.getNestedTypes().get(1);
    internal_static_WISE_GridProto_wcsData_binaryData_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_wcsData_binaryData_descriptor,
        new java.lang.String[] { "Data", "DataValid", "IsZipped", });
    com.google.protobuf.WrappersProto.getDescriptor();
  }

  // @@protoc_insertion_point(outer_class_scope)
}
