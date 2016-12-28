using System;
using System.Runtime.InteropServices;

namespace Klawr.UnrealEngine
{
    /// <summary>
    /// A vector representing a position or direction in 3D space.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct FVector : IEquatable<FVector>
    {
        /// <summary>
        /// Default tolerance to use when comparing floats.
        /// </summary>
        public const float Tolerance = 0.001f;

        /// <summary>
        /// Positive unit vector in Z axis.
        /// </summary>
        public static readonly FVector Up = new FVector(0.0f, 0.0f, 1.0f);

        /// <summary>
        /// Negative unit vector in Z axis.
        /// </summary>
        public static readonly FVector Down = new FVector(0.0f, 0.0f, -1.0f);

        /// <summary>
        /// Positive unit vector in X axis.
        /// </summary>
        public static readonly FVector Forward = new FVector(1.0f, 0.0f, 0.0f);

        /// <summary>
        /// Negative unit vector in X axis.
        /// </summary>
        public static readonly FVector Backward = new FVector(-1.0f, 0.0f, 0.0f);

        /// <summary>
        /// Negative unit vector in Y axis.
        /// </summary>
        public static readonly FVector Right = new FVector(0.0f, -1.0f, 0.0f);

        /// <summary>
        /// Positive unit vector in Y axis.
        /// </summary>
        public static readonly FVector Left = new FVector(0.0f, 1.0f, 0.0f);

        public float X;
        public float Y;
        public float Z;

        public FVector(float inF) { X = inF; Y = inF; Z = inF; }
        public FVector(FLinearColor inColor) { X = inColor.R; Y = inColor.G; Z = inColor.B; }
        public FVector(FVector2D inV, float inZ) { X = inV.X; Y = inV.Y; Z = inZ; }
        public FVector(float inX, float inY, float inZ) { X = inX; Y = inY; Z = inZ; }

        #region Operator Overloads

        public static FVector operator +(FVector inLeft, FVector inRight)
        {
            return new FVector(inLeft.X + inRight.X, inLeft.Y + inRight.Y, inLeft.Z + inRight.Z);
        }

        public static FVector operator +(FVector inLeft, float inRight)
        {
            return new FVector(inLeft.X + inRight, inLeft.Y + inRight, inLeft.Z + inRight);
        }

        public static FVector operator -(FVector inLeft, FVector inRight)
        {
            return new FVector(inLeft.X - inRight.X, inLeft.Y - inRight.Y, inLeft.Z - inRight.Z);
        }

        public static FVector operator -(FVector inLeft, float inRight)
        {
            return new FVector(inLeft.X - inRight, inLeft.Y - inRight, inLeft.Z - inRight);
        }

        public static FVector operator *(FVector inLeft, float inRight)
        {
            return new FVector(inLeft.X * inRight, inLeft.Y * inRight, inLeft.Z * inRight);
        }

        public static FVector operator *(FVector inLeft, FVector inRight)
        {
            return new FVector(inLeft.X * inRight.X, inLeft.Y * inRight.Y, inLeft.Z * inRight.Z);
        }

        public static FVector operator /(FVector inLeft, float inRight)
        {
            return new FVector(inLeft.X / inRight, inLeft.Y / inRight, inLeft.Z / inRight);
        }

        public static FVector operator /(FVector inLeft, FVector inRight)
        {
            return new FVector(inLeft.X / inRight.X, inLeft.Y / inRight.Y, inLeft.Z / inRight.Z);
        }

        public static FVector operator %(FVector inLeft, float inRight)
        {
            return new FVector(inLeft.X % inRight, inLeft.Y % inRight, inLeft.Z % inRight);
        }

        public static FVector operator %(FVector inLeft, FVector inRight)
        {
            return new FVector(inLeft.X % inRight.X, inLeft.Y % inRight.Y, inLeft.Z % inRight.Z);
        }

        public static FVector operator <<(FVector inLeft, int inRight)
        {
            inRight %= 3;
            while (inRight-- > 0) { inLeft = inLeft.YZX; }
            return inLeft;
        }

        public static FVector operator >>(FVector inLeft, int inRight)
        {
            inRight %= 3;
            while (inRight-- > 0) { inLeft = inLeft.ZXY; }
            return inLeft;
        }

        /// <summary>
        /// Calculate cross product between this and another vector.
        /// </summary>
        /// <param name="inLeft"></param>
        /// <param name="inRight"></param>
        /// <returns></returns>
        public static FVector operator ^(FVector inLeft, FVector inRight)
        {
            return new FVector(
                inLeft.Y * inRight.Z - inLeft.Z * inRight.Y,
                inLeft.X * inRight.Z - inLeft.Z * inRight.X,
                inLeft.X * inRight.Y - inLeft.Y * inRight.X
            );
        }

        /// <summary>
        /// Calculate dot product between this and another vector.
        /// </summary>
        /// <param name="inLeft"></param>
        /// <param name="inRight"></param>
        /// <returns></returns>
        public static float operator |(FVector inLeft, FVector inRight)
        {
            return inLeft.X * inRight.X + inLeft.Y * inRight.Y + inLeft.Z + inRight.Z;
        }

        public static bool operator ==(FVector inLeft, FVector inRight)
        {
            return inLeft.X == inRight.X && inLeft.Y == inRight.Y && inLeft.Z == inRight.Z;
        }

        public static bool operator ==(FVector inLeft, float inRight)
        {
            return inLeft.X == inRight && inLeft.Y == inRight && inLeft.Z == inRight;
        }

        public static bool operator !=(FVector inLeft, FVector inRight)
        {
            return inLeft.X != inRight.X || inLeft.Y != inRight.Y || inLeft.Z != inRight.Z;
        }

        public static bool operator !=(FVector inLeft, float inRight)
        {
            return inLeft.X != inRight || inLeft.Y != inRight || inLeft.Z != inRight;
        }

        public static bool operator >(FVector inLeft, float inRight)
        {
            return inLeft.X > inRight && inLeft.Y > inRight && inLeft.Z > inRight;
        }

        public static bool operator >=(FVector inLeft, float inRight)
        {
            return inLeft.X >= inRight && inLeft.Y >= inRight && inLeft.Z >= inRight;
        }

        public static bool operator <(FVector inLeft, float inRight)
        {
            return inLeft.X < inRight && inLeft.Y < inRight && inLeft.Z < inRight;
        }

        public static bool operator <=(FVector inLeft, float inRight)
        {
            return inLeft.X <= inRight && inLeft.Y <= inRight && inLeft.Z <= inRight;
        }

        public override bool Equals(object obj)
        {
            return base.Equals(obj) || (obj is FVector) && ((FVector)obj) == this;
        }

        public override int GetHashCode()
        {
            return X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode();
        }

        public override string ToString()
        {
            return $"FVector[${X},${Y},${Z}]";
        }

        #endregion

        #region Swizzle

        // Rotate left
        public FVector YZX
        {
            get
            {
                return new FVector(Y, Z, X);
            }
            set
            {
                Y = value.X;
                Z = value.Y;
                X = value.Z;
            }
        }

        // Rotate right
        public FVector ZXY
        {
            get
            {
                return new FVector(Z, X, Y);
            }
            set
            {
                Z = value.X;
                X = value.Y;
                Y = value.Z;
            }
        }

        // Reverse
        public FVector ZYX
        {
            get
            {
                return new FVector(Z, Y, Z);
            }
            set
            {
                Z = value.X;
                Y = value.Y;
                X = value.Z;
            }
        }

        // Reverse, rotate left
        public FVector YXZ
        {
            get
            {
                return new FVector(Y, X, Z);
            }
            set
            {
                Y = value.X;
                X = value.Y;
                Z = value.Z;
            }
        }

        // Reverse, rotate right
        public FVector XZY
        {
            get
            {
                return new FVector(X, Z, Y);
            }
            set
            {
                X = value.X;
                Z = value.Y;
                Y = value.Z;
            }
        }

        #endregion

        #region IEquatable<FVector>

        /// <summary>
        /// Check against another vector for equality.
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        public bool Equals(FVector other)
        {
            return this == other;
        }

        /// <summary>
        /// Check against another vector for equality, within specified error limits.
        /// </summary>
        /// <param name="other"></param>
        /// <param name="tolerance"></param>
        /// <returns></returns>
        public bool Equals(FVector other, float tolerance = Tolerance)
        {
            float dc = other.X - X;
            if (dc > Tolerance || dc < -Tolerance) return false;
            dc = other.Y - Y;
            if (dc > Tolerance || dc < -Tolerance) return false;
            dc = other.Z - Z;
            return dc <= Tolerance && dc >= -Tolerance;
        }

        #endregion

        /// <summary>
        /// Gets or sets a component of this vector.
        /// </summary>
        /// <param name="componentIndex"></param>
        /// <returns></returns>
        public float this[int componentIndex]
        {
            get
            {
                return componentIndex == 0 ? X : componentIndex == 1 ? Y : componentIndex == 2 ? Z : throw new IndexOutOfRangeException();
            }
            set
            {
                switch (componentIndex)
                {
                    case 0: X = value; break;
                    case 1: Y = value; break;
                    case 2: Z = value; break;
                    default: throw new IndexOutOfRangeException();
                }

            }
        }

        /// <summary>
        /// Add a vector to this and clamp the result in a cube.
        /// </summary>
        /// <param name="inV"></param>
        /// <param name="inRadius"></param>
        public void AddBounded(FVector inV, float inRadius)
        {
            this += inV;
            X = X > inRadius ? inRadius : X < -inRadius ? -inRadius : X;
            Y = Y > inRadius ? inRadius : Y < -inRadius ? -inRadius : Y;
            Z = Z > inRadius ? inRadius : Z < -inRadius ? -inRadius : Z;
        }

        /// <summary>
        /// Checks whether all components of this vector are the same, within a tolerance.
        /// </summary>
        /// <param name="tolerance"></param>
        /// <returns></returns>
        public bool AllComponentsEqual(float tolerance = Tolerance)
        {
            float dc = X - Y;
            if (dc > tolerance || dc < -tolerance) return false;
            dc = X - Z;
            return dc <= tolerance && dc >= -tolerance;
        }

        /// <summary>
        /// Get a copy of this vector, clamped inside of a cube. A copy of this vector, bound by cube.
        /// </summary>
        /// <param name="inRadius"></param>
        /// <returns></returns>
        public  FVector BoundToCube(float inRadius)
        {
            FVector copy;
            copy.X = X > inRadius ? inRadius : X < -inRadius ? -inRadius : X;
            copy.Y = Y > inRadius ? inRadius : Y < -inRadius ? -inRadius : Y;
            copy.Z = Z > inRadius ? inRadius : Z < -inRadius ? -inRadius : Z;
            return copy;
        }

        /// <summary>
        /// Normalize this vector in-place if it is large enough, set it to (0,0,0) otherwise.
        /// </summary>
        /// <returns></returns>
        public bool Normalize()
        {
            float currentSize = Size;
            if (currentSize < Tolerance)
            {
                X = 0.0f; Y = 0.0f; Z = 0.0f;
                return false;
            }
            this *= (1.0f / currentSize);
            return true;
        }

        /// <summary>
        /// Sets the components of this vector.
        /// </summary>
        /// <param name="inX"></param>
        /// <param name="inY"></param>
        /// <param name="inZ"></param>
        public void Set(float inX, float inY, float inZ) { X = inX; Y = inY; Z = inZ; }

        /// <summary>
        /// Sets the 2D components of this vector.
        /// </summary>
        /// <param name="inX"></param>
        /// <param name="inY"></param>
        /// <param name="inZ"></param>
        public void Set(float inX, float inY) { X = inX; Y = inY; }

        /// <summary>
        /// Checks whether all components of the vector are exactly zero.
        /// </summary>
        public bool IsZero
        {
            get
            {
                return this == 0.0f;
            }
        }

        /// <summary>
        /// Check if the vector is of unit length.
        /// </summary>
        public bool IsUnit
        {
            get
            {
                float sizeSquared = SizeSquared;
                return sizeSquared < Tolerance && sizeSquared > -Tolerance;
            }
        }

        /// <summary>
        /// Gets or sets the length (magnitude) of this vector.
        /// </summary>
        public float Size
        {
            get
            {
                return (float)System.Math.Sqrt(X * X + Y * Y + Z * Z);
            }
            set
            {
                if (value < Tolerance && value > -Tolerance)
                {
                    X = 0.0f; Y = 0.0f; Z = 0.0f;
                    return;
                }
                float currentSize = Size;
                if (currentSize < Tolerance) return;
                this *= (value / currentSize);
            }
        }

        /// <summary>
        /// Gets or sets the squared length (magnitude) of this vector.
        /// </summary>
        public float SizeSquared
        {
            get
            {
                return X * X + Y * Y + Z * Z;
            }
            set
            {
                if (value < Tolerance && value > -Tolerance)
                {
                    X = 0.0f; Y = 0.0f; Z = 0.0f;
                    return;
                }
                // Will this break because the following equation is linear and we're working with a squared size?
                float currentSize = SizeSquared;
                if (currentSize < Tolerance) return;
                this *= (value / currentSize);
            }
        }

        /// <summary>
        /// Gets or sets the length (magnitude) of the 2D components of this vector.
        /// </summary>
        public float Size2D
        {
            get
            {
                return (float)System.Math.Sqrt(X * X + Y * Y);
            }
            set
            {
                if (value < Tolerance && value > -Tolerance)
                {
                    X = 0.0f; Y = 0.0f;
                    return;
                }
                float currentSize = Size2D;
                if (currentSize < Tolerance) return;
                float mul = (value / currentSize);
                X *= mul; Y *= mul;
            }
        }

        /// <summary>
        /// Gets or sets the squared length (magnitude) of the 2D components of this vector.
        /// </summary>
        public float Size2DSquared
        {
            get
            {
                return (float)System.Math.Sqrt(X * X + Y * Y);
            }
            set
            {
                if (value < Tolerance && value > -Tolerance)
                {
                    X = 0.0f; Y = 0.0f;
                    return;
                }
                float currentSize = Size2D;
                if (currentSize < Tolerance) return;
                float mul = (value / currentSize);
                X *= mul; Y *= mul;
            }
        }

    }
}
