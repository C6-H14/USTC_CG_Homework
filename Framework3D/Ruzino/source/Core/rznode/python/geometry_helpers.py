"""
Geometry type helpers for node system integration.

This module provides helper functions to convert between Python Geometry objects
and meta_any containers used by the node system.
"""

try:
    import geometry_py as geom
    import nodes_core_py as core
    GEOMETRY_AVAILABLE = True
except ImportError:
    GEOMETRY_AVAILABLE = False


def geometry_to_meta_any(geometry_obj):
    """
    Convert a Geometry object to meta_any.
    
    Note: This is a workaround since Geometry objects are passed through
    the node system as opaque pointers. The actual conversion happens in C++.
    """
    if not GEOMETRY_AVAILABLE:
        raise ImportError("rzgeometry_py not available")
    
    # The Geometry object is already wrapped in the correct way by the node system
    # We just need to ensure it's the right type
    if not isinstance(geometry_obj, geom.Geometry):
        raise TypeError(f"Expected Geometry object, got {type(geometry_obj)}")
    
    # For now, return the object as-is
    # The node system will handle it through C++ internally
    return geometry_obj


def meta_any_to_geometry(meta_any_obj):
    """
    Convert a meta_any to a Geometry object.
    
    Note: This is a workaround. The actual conversion happens in C++.
    """
    if not GEOMETRY_AVAILABLE:
        raise ImportError("rzgeometry_py not available")
    
    # The meta_any should already contain a Geometry object
    # This is handled by the C++ node execution system
    return meta_any_obj


def is_geometry(obj):
    """Check if an object is a Geometry instance."""
    if not GEOMETRY_AVAILABLE:
        return False
    return isinstance(obj, geom.Geometry)


# Export helper functions
__all__ = [
    'geometry_to_meta_any',
    'meta_any_to_geometry',
    'is_geometry',
    'GEOMETRY_AVAILABLE'
]
