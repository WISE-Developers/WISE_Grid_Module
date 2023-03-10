// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: cwfgmGrid.proto

package ca.wise.grid.proto;

public final class CwfgmGridOuterClass {
  private CwfgmGridOuterClass() {}
  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistryLite registry) {
  }

  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistry registry) {
    registerAllExtensions(
        (com.google.protobuf.ExtensionRegistryLite) registry);
  }
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmGrid_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmGrid_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmGrid_ElevationFile_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmGrid_ElevationFile_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmGrid_FuelMapFile_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmGrid_FuelMapFile_fieldAccessorTable;
  static final com.google.protobuf.Descriptors.Descriptor
    internal_static_WISE_GridProto_CwfgmGrid_ProjectionFile_descriptor;
  static final 
    com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internal_static_WISE_GridProto_CwfgmGrid_ProjectionFile_fieldAccessorTable;

  public static com.google.protobuf.Descriptors.FileDescriptor
      getDescriptor() {
    return descriptor;
  }
  private static  com.google.protobuf.Descriptors.FileDescriptor
      descriptor;
  static {
    java.lang.String[] descriptorData = {
      "\n\017cwfgmGrid.proto\022\016WISE.GridProto\032\nmath." +
      "proto\032\rwcsData.proto\032\036google/protobuf/wr" +
      "appers.proto\"\251\007\n\tCwfgmGrid\022\017\n\007version\030\001 " +
      "\001(\005\022+\n\005xSize\030\002 \001(\0132\034.google.protobuf.UIn" +
      "t32Value\022+\n\005ySize\030\003 \001(\0132\034.google.protobu" +
      "f.UInt32Value\022\037\n\txLLCorner\030\004 \001(\0132\014.Math." +
      "Double\022\037\n\tyLLCorner\030\005 \001(\0132\014.Math.Double\022" +
      " \n\nresolution\030\006 \001(\0132\014.Math.Double\022$\n\nllL" +
      "ocation\030\007 \001(\0132\020.Math.Coordinate\022%\n\017nodat" +
      "aElevation\030\010 \001(\0132\014.Math.Double\0226\n\007fuelMa" +
      "p\030\t \001(\0132%.WISE.GridProto.CwfgmGrid.FuelM" +
      "apFile\022:\n\televation\030\n \001(\0132\'.WISE.GridPro" +
      "to.CwfgmGrid.ElevationFile\022<\n\nprojection" +
      "\030\013 \001(\0132(.WISE.GridProto.CwfgmGrid.Projec" +
      "tionFile\032j\n\rElevationFile\022)\n\010contents\030\001 " +
      "\001(\0132\027.WISE.GridProto.wcsData\022.\n\010filename" +
      "\030\002 \001(\0132\034.google.protobuf.StringValue\032\226\001\n" +
      "\013FuelMapFile\022)\n\010contents\030\001 \001(\0132\027.WISE.Gr" +
      "idProto.wcsData\022,\n\006header\030\002 \001(\0132\034.google" +
      ".protobuf.StringValue\022.\n\010filename\030\003 \001(\0132" +
      "\034.google.protobuf.StringValue\032\310\001\n\016Projec" +
      "tionFile\022.\n\010contents\030\001 \001(\0132\034.google.prot" +
      "obuf.StringValue\022)\n\003wkt\030\002 \001(\0132\034.google.p" +
      "rotobuf.StringValue\022+\n\005units\030\003 \001(\0132\034.goo" +
      "gle.protobuf.StringValue\022.\n\010filename\030\004 \001" +
      "(\0132\034.google.protobuf.StringValueB\'\n\022ca.w" +
      "ise.grid.protoP\001\252\002\016WISE.GridProtob\006proto" +
      "3"
    };
    descriptor = com.google.protobuf.Descriptors.FileDescriptor
      .internalBuildGeneratedFileFrom(descriptorData,
        new com.google.protobuf.Descriptors.FileDescriptor[] {
          ca.hss.math.proto.Math.getDescriptor(),
          ca.wise.grid.proto.wcsDataPackage.getDescriptor(),
          com.google.protobuf.WrappersProto.getDescriptor(),
        });
    internal_static_WISE_GridProto_CwfgmGrid_descriptor =
      getDescriptor().getMessageTypes().get(0);
    internal_static_WISE_GridProto_CwfgmGrid_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmGrid_descriptor,
        new java.lang.String[] { "Version", "XSize", "YSize", "XLLCorner", "YLLCorner", "Resolution", "LlLocation", "NodataElevation", "FuelMap", "Elevation", "Projection", });
    internal_static_WISE_GridProto_CwfgmGrid_ElevationFile_descriptor =
      internal_static_WISE_GridProto_CwfgmGrid_descriptor.getNestedTypes().get(0);
    internal_static_WISE_GridProto_CwfgmGrid_ElevationFile_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmGrid_ElevationFile_descriptor,
        new java.lang.String[] { "Contents", "Filename", });
    internal_static_WISE_GridProto_CwfgmGrid_FuelMapFile_descriptor =
      internal_static_WISE_GridProto_CwfgmGrid_descriptor.getNestedTypes().get(1);
    internal_static_WISE_GridProto_CwfgmGrid_FuelMapFile_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmGrid_FuelMapFile_descriptor,
        new java.lang.String[] { "Contents", "Header", "Filename", });
    internal_static_WISE_GridProto_CwfgmGrid_ProjectionFile_descriptor =
      internal_static_WISE_GridProto_CwfgmGrid_descriptor.getNestedTypes().get(2);
    internal_static_WISE_GridProto_CwfgmGrid_ProjectionFile_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
        internal_static_WISE_GridProto_CwfgmGrid_ProjectionFile_descriptor,
        new java.lang.String[] { "Contents", "Wkt", "Units", "Filename", });
    ca.hss.math.proto.Math.getDescriptor();
    ca.wise.grid.proto.wcsDataPackage.getDescriptor();
    com.google.protobuf.WrappersProto.getDescriptor();
  }

  // @@protoc_insertion_point(outer_class_scope)
}
