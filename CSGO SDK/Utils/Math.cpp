#include "Math.h"
#include "../source.hpp"
#include "../SDK/Classes/Player.hpp"
#include "../SDK/CVariables.hpp"

#include <algorithm>
#include <numeric>
#include <xmmintrin.h>

static Vector DirBetweenLines( const Vector& a, const Vector& b, const Vector& c, const Vector& d ) {
	const Vector d1 = ( b - a );
	const Vector d2 = ( d - c );

	const Vector cross = d1.Cross( d2 );

	const Vector cross1 = d1.Cross( cross );
	const Vector cross2 = d2.Cross( cross );

	const Vector sp = c + d2 * Math::Clamp( ( a - c ).Dot( cross1 ) / ( d2.Dot( cross1 ) ), 0.f, 1.f );
	const Vector ep = a + d1 * Math::Clamp( ( c - a ).Dot( cross2 ) / ( d1.Dot( cross2 ) ), 0.f, 1.f );

	return ep - sp;
}

typedef __declspec( align( 16 ) ) union {
	float f[ 4 ];
	__m128 v;
} m128;

__forceinline __m128 sqrt_ps( const __m128 squared ) {
	return _mm_sqrt_ps( squared );
}

bool Math::IntersectSegmentToSegment( Vector s1, Vector s2, Vector k1, Vector k2, float radius ) {
	static auto constexpr epsilon = 0.00000001;

	auto u = s2 - s1;
	auto v = k2 - k1;
	const auto w = s1 - k1;

	const auto a = u.Dot( u );
	const auto b = u.Dot( v );
	const auto c = v.Dot( v );
	const auto d = u.Dot( w );
	const auto e = v.Dot( w );
	const auto D = a * c - b * b;
	float sn, sd = D;
	float tn, td = D;

	if( D < epsilon ) {
		sn = 0.0;
		sd = 1.0;
		tn = e;
		td = c;
	}
	else {
		sn = b * e - c * d;
		tn = a * e - b * d;

		if( sn < 0.0 ) {
			sn = 0.0;
			tn = e;
			td = c;
		}
		else if( sn > sd ) {
			sn = sd;
			tn = e + b;
			td = c;
		}
	}

	if( tn < 0.0 ) {
		tn = 0.0;

		if( -d < 0.0 )
			sn = 0.0;
		else if( -d > a )
			sn = sd;
		else {
			sn = -d;
			sd = a;
		}
	}
	else if( tn > td ) {
		tn = td;

		if( -d + b < 0.0 )
			sn = 0;
		else if( -d + b > a )
			sn = sd;
		else {
			sn = -d + b;
			sd = a;
		}
	}

	const float sc = abs( sn ) < epsilon ? 0.0 : sn / sd;
	const float tc = abs( tn ) < epsilon ? 0.0 : tn / td;

	m128 n;
	auto dp = w + u * sc - v * tc;
	n.f[ 0 ] = dp.Dot( dp );
	const auto calc = sqrt_ps( n.v );
	auto shit = reinterpret_cast< const m128* >( &calc )->f[ 0 ];
	//printf( "shit %f | rad %f\n", shit, radius );
	return shit < radius;
}

bool Math::CapsuleCollider::Intersect( const Vector& start, const Vector& end ) const {
#if 1
	static auto constexpr epsilon = 0.00000001f;

	const auto s1 = min;
	const auto s2 = max;
	const auto k1 = start;
	const auto k2 = end;

	auto u = s2 - s1;
	auto v = k2 - k1;
	const auto w = s1 - k1;

	const auto a = u.Dot( u );
	const auto b = u.Dot( v );
	const auto c = v.Dot( v );
	const auto d = u.Dot( w );
	const auto e = v.Dot( w );
	const auto D = a * c - b * b;
	float sn, sd = D;
	float tn, td = D;

	if( D < epsilon ) {
		sn = 0.0f;
		sd = 1.0f;
		tn = e;
		td = c;
	}
	else {
		sn = b * e - c * d;
		tn = a * e - b * d;

		if( sn < 0.0f ) {
			sn = 0.0f;
			tn = e;
			td = c;
		}
		else if( sn > sd ) {
			sn = sd;
			tn = e + b;
			td = c;
		}
	}

	if( tn < 0.0f ) {
		tn = 0.0f;

		if( -d < 0.0f )
			sn = 0.0f;
		else if( -d > a )
			sn = sd;
		else {
			sn = -d;
			sd = a;
		}
	}
	else if( tn > td ) {
		tn = td;

		if( -d + b < 0.0f )
			sn = 0.f;
		else if( -d + b > a )
			sn = sd;
		else {
			sn = -d + b;
			sd = a;
		}
	}

	const float sc = abs( sn ) < epsilon ? 0.0f : sn / sd;
	const float tc = abs( tn ) < epsilon ? 0.0f : tn / td;

	m128 n;
	auto dp = w + u * sc - v * tc;
	n.f[ 0 ] = dp.Dot( dp );
	const auto calc = sqrt_ps( n.v );
	return radius < reinterpret_cast< const m128* >( &calc )->f[ 0 ];

	//auto dp = w + u * sc - v * tc;
	//auto scale = dp.Dot( dp );
	//return scale < radius * radius;
#endif

#if 0
	const Vector dir = DirBetweenLines( min, max, start, end );
	return dir.LengthSquared( ) <= radius * radius;


	return IntersectSegmentCapsule( start, end, min, max, radius );
#endif
}

bool Math::IntersectSegmentSphere( const Vector& vecRayOrigin, const Vector& vecRayDelta, const Vector& vecSphereCenter, float flRadius ) {
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	Vector vecSphereToRay = vecRayOrigin - vecSphereCenter;

	float a = vecRayDelta.Dot( vecRayDelta );

	// This would occur in the case of a zero-length ray
	if( a == 0.0f )
		return vecSphereToRay.LengthSquared( ) <= flRadius * flRadius;

	float b = 2.f * vecSphereToRay.Dot( vecRayDelta );
	float c = vecSphereToRay.Dot( vecSphereToRay ) - flRadius * flRadius;
	float flDiscrim = b * b - 4.f * a * c;
	return flDiscrim >= 0.0f;
}


bool Math::IntersectSegmentCapsule( const Vector& start, const Vector& end, const Vector& min, const Vector& max, float radius ) {
	Vector d = max - min, m = start - min, n = end - start;
	float md = m.Dot( d );
	float nd = n.Dot( d );
	float dd = d.Dot( d );

	if( md < 0.0f && md + nd < 0.0f ) {
		return IntersectSegmentSphere( start, n, min, radius );
	}
	if( md > dd && md + nd > dd ) {
		return IntersectSegmentSphere( start, n, max, radius );
	}

	float t = 0.0f;
	float nn = n.Dot( n );
	float mn = m.Dot( n );
	float a = dd * nn - nd * nd;
	float k = m.Dot( m ) - radius * radius;
	float c = dd * k - md * md;
	if( std::fabsf( a ) < FLT_EPSILON ) {
		if( c > 0.0f )
			return 0;
		if( md < 0.0f )
			IntersectSegmentSphere( start, n, min, radius );
		else if( md > dd )
			IntersectSegmentSphere( start, n, max, radius );
		else
			t = 0.0f;
		return true;
	}
	float b = dd * mn - nd * md;
	float discr = b * b - a * c;
	if( discr < 0.0f )
		return false;

	t = ( -b - sqrt( discr ) ) / a;
	float t0 = t;
	if( md + t * nd < 0.0f ) {
		return IntersectSegmentSphere( start, n, min, radius );
	}
	else if( md + t * nd > dd ) {

		return IntersectSegmentSphere( start, n, max, radius );
	}
	t = t0;
	return t > 0.0f && t < 1.0f;
}

bool Math::IntersectionBoundingBox( const Vector& src, const Vector& dir, const Vector& min, const Vector& max, Vector* hit_point ) {
	/*
		 Fast Ray-Box Intersection
		 by Andrew Woo
		 from "Graphics Gems", Academic Press, 1990
	 */

	constexpr auto NUMDIM = 3;
	constexpr auto RIGHT = 0;
	constexpr auto LEFT = 1;
	constexpr auto MIDDLE = 2;

	bool inside = true;
	char quadrant[ NUMDIM ];
	int i;

	// Rind candidate planes; this loop can be avoided if
	// rays cast all from the eye(assume perpsective view)
	Vector candidatePlane;
	for( i = 0; i < NUMDIM; i++ ) {
		if( src[ i ] < min[ i ] ) {
			quadrant[ i ] = LEFT;
			candidatePlane[ i ] = min[ i ];
			inside = false;
		}
		else if( src[ i ] > max[ i ] ) {
			quadrant[ i ] = RIGHT;
			candidatePlane[ i ] = max[ i ];
			inside = false;
		}
		else {
			quadrant[ i ] = MIDDLE;
		}
	}

	// Ray origin inside bounding box
	if( inside ) {
		if( hit_point )
			*hit_point = src;
		return true;
	}

	// Calculate T distances to candidate planes
	Vector maxT;
	for( i = 0; i < NUMDIM; i++ ) {
		if( quadrant[ i ] != MIDDLE && dir[ i ] != 0.f )
			maxT[ i ] = ( candidatePlane[ i ] - src[ i ] ) / dir[ i ];
		else
			maxT[ i ] = -1.f;
	}

	// Get largest of the maxT's for final choice of intersection
	int whichPlane = 0;
	for( i = 1; i < NUMDIM; i++ ) {
		if( maxT[ whichPlane ] < maxT[ i ] )
			whichPlane = i;
	}

	// Check final candidate actually inside box
	if( maxT[ whichPlane ] < 0.f )
		return false;

	for( i = 0; i < NUMDIM; i++ ) {
		if( whichPlane != i ) {
			float temp = src[ i ] + maxT[ whichPlane ] * dir[ i ];
			if( temp < min[ i ] || temp > max[ i ] ) {
				return false;
			}
			else if( hit_point ) {
				( *hit_point )[ i ] = temp;
			}
		}
		else if( hit_point ) {
			( *hit_point )[ i ] = candidatePlane[ i ];
		}
	}

	// ray hits box
	return true;
}

void Math::Rotate( std::array< Vector2D, 3 >& points, float rotation ) {
	const auto points_center = ( points.at( 0 ) + points.at( 1 ) + points.at( 2 ) ) / 3;
	for( auto& point : points ) {
		point -= points_center;

		const auto temp_x = point.x;
		const auto temp_y = point.y;

		const auto theta = ToRadians( rotation );
		float c, s;
		DirectX::XMScalarSinCos( &s, &c, theta );

		point.x = temp_x * c - temp_y * s;
		point.y = temp_x * s + temp_y * c;

		point += points_center;
	}
}

void Math::angle_vectors(const QAngle& angles, Vector* forward, Vector* right, Vector* up)
{
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles.x), &sp, &cp);
	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.z), &sr, &cr);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;
	}

	if (up)
	{
		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
}


void Math::AngleVectors( const QAngle& angles, Vector& forward, Vector& right, Vector& up ) {
	float sr, sp, sy, cr, cp, cy;

	SinCos( DEG2RAD( angles[ 1 ] ), &sy, &cy );
	SinCos( DEG2RAD( angles[ 0 ] ), &sp, &cp );
	SinCos( DEG2RAD( angles[ 2 ] ), &sr, &cr );

	forward.x = ( cp * cy );
	forward.y = ( cp * sy );
	forward.z = ( -sp );
	right.x = ( -1 * sr * sp * cy + -1 * cr * -sy );
	right.y = ( -1 * sr * sp * sy + -1 * cr * cy );
	right.z = ( -1 * sr * cp );
	up.x = ( cr * sp * cy + -sr * -sy );
	up.y = ( cr * sp * sy + -sr * cy );
	up.z = ( cr * cp );
}

void Math::VectorAngles( const Vector& forward, QAngle &angles ) {
	float tmp, yaw, pitch;

	if( forward[ 2 ] == 0.0f && forward[ 0 ] == 0.0f ) {
		yaw = 0;

		if( forward[ 2 ] > 0.0f )
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else {
		yaw = ( atan2( forward[ 1 ], forward[ 0 ] ) * 180 / M_PI );
		if( yaw < 0 )
			yaw += 360;

		tmp = sqrt( forward[ 0 ] * forward[ 0 ] + forward[ 1 ] * forward[ 1 ] );
		pitch = ( atan2( -forward[ 2 ], tmp ) * 180 / M_PI );
		if( pitch < 0 )
			pitch += 360;
	}

	pitch -= floorf( pitch / 360.0f + 0.5f ) * 360.0f;
	yaw -= floorf( yaw / 360.0f + 0.5f ) * 360.0f;

	if( pitch > 89.0f )
		pitch = 89.0f;
	else if( pitch < -89.0f )
		pitch = -89.0f;

	angles.x = pitch;
	angles.y = yaw;
	angles.z = 0;
}

void Math::VectorAngles( const Vector& forward, Vector& angles ) {
	float tmp, yaw, pitch;

	if( forward[ 1 ] == 0 && forward[ 0 ] == 0 ) {
		yaw = 0;
		if( forward[ 2 ] > 0 )
			pitch = 270;
		else
			pitch = 90;
	}
	else {
		yaw = ( atan2( forward[ 1 ], forward[ 0 ] ) * 180 / M_PI );
		if( yaw < 0 )
			yaw += 360;

		tmp = sqrt( forward[ 0 ] * forward[ 0 ] + forward[ 1 ] * forward[ 1 ] );
		pitch = ( atan2( -forward[ 2 ], tmp ) * 180 / M_PI );
		if( pitch < 0 )
			pitch += 360;
	}

	angles[ 0 ] = pitch;
	angles[ 1 ] = yaw;
	angles[ 2 ] = 0;
}

void Math::SinCos( float a, float* s, float* c ) {
	*s = sin( a );
	*c = cos( a );
}

void Math::AngleVectors( const QAngle& angles, Vector* forward ) {
	float	sp, sy, cp, cy;

	SinCos( DEG2RAD( angles[ 1 ] ), &sy, &cy );
	SinCos( DEG2RAD( angles[ 0 ] ), &sp, &cp );

	forward[0].x = cp * cy;
	forward[1].y = cp * sy;
	forward[2].z = -sp;
}

void Math::AngleVectors( const QAngle & angles, Vector & forward ) {
	float	sp, sy, cp, cy;

	SinCos( DEG2RAD( angles[ 1 ] ), &sy, &cy );
	SinCos( DEG2RAD( angles[ 0 ] ), &sp, &cp );

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

float Math::get_fov(const QAngle& angles, const QAngle& target)
{
	Vector ang, aim;

	angle_vectors(angles, &aim);
	angle_vectors(target, &ang);

	return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSquared()));
}


float Math::GetFov( const QAngle& viewAngle, const Vector& start, const Vector& end ) {

	Vector dir, fw;

	// get direction and normalize.
	dir = ( end - start ).Normalized( );

	// get the forward direction vector of the view angles.
	AngleVectors( viewAngle, fw );

	// get the angle between the view angles forward directional vector and the target location.
	return std::max( RAD2DEG( std::acos( fw.Dot( dir ) ) ), 0.f );
}

float Math::VecGetFov(const Vector& src, const Vector& dst)
{
	Vector v_src = Vector();
	Vecanglevectors(src, &v_src);

	Vector v_dst = Vector();
	Vecanglevectors(dst, &v_dst);

	float result = RAD2DEG(acos(v_dst.Dot(v_src) / v_dst.LengthSquared()));

	if (!isfinite(result) || isinf(result) || isnan(result))
		result = 0.0f;

	return result;
}

float Math::normalize_float(float angle)
{
	auto revolutions = angle / 360.f;
	if (angle > 180.f || angle < -180.f)
	{
		revolutions = round(abs(revolutions));
		if (angle < 0.f)
			angle = (angle + 360.f * revolutions);
		else
			angle = (angle - 360.f * revolutions);
		return angle;
	}
	return angle;
}


void Math::NormalizeAngle(float& angle) {
	float rot;

	// bad number.
	if (!std::isfinite(angle)) {
		angle = 0.f;
		return;
	}

	// no need to normalize this angle.
	if (angle >= -180.f && angle <= 180.f)
		return;

	// get amount of rotations needed.
	rot = std::round(std::abs(angle / 360.f));

	// normalize.
	angle = (angle < 0.f) ? angle + (360.f * rot) : angle - (360.f * rot);
}


float Math::AngleNormalize( float angle ) {
	if( angle > 180.f || angle < -180.f ) {
		auto revolutions = round( abs( angle / 360.f ) );

		if( angle < 0.f )
			angle = angle + 360.f * revolutions;
		else
			angle = angle - 360.f * revolutions;
	}

	return angle;
}
float Math::ApproachAngle( float target, float value, float speed ) {
	target = ( target * 182.04445f ) * 0.0054931641f;
	value = ( value * 182.04445f ) * 0.0054931641f;

	// Speed is assumed to be positive
	if( speed < 0 )
		speed = -speed;

	float delta = target - value;
	if( delta < -180.0f )
		delta += 360.0f;
	else if( delta > 180.0f )
		delta -= 360.0f;

	if( delta > speed )
		value += speed;
	else if( delta < -speed )
		value -= speed;
	else
		value = target;

	return value;
}
void Math::VectorTransform( const Vector& in1, const matrix3x4_t& in2, Vector& out ) {
	out[ 0 ] = in1.Dot( in2[ 0 ] ) + in2[ 0 ][ 3 ];
	out[ 1 ] = in1.Dot( in2[ 1 ] ) + in2[ 1 ][ 3 ];
	out[ 2 ] = in1.Dot( in2[ 2 ] ) + in2[ 2 ][ 3 ];
}

void Math::VectorTransform2(Vector& in1, matrix3x4_t& in2, Vector& out)
{
	/*out.x = in1.Dot(in2.m_flMatVal[0]) + in2.m_flMatVal[0][3];
	out.y = in1.Dot(in2.m_flMatVal[1]) + in2.m_flMatVal[1][3];
	out.z = in1.Dot(in2.m_flMatVal[2]) + in2.m_flMatVal[2][3];*/
	out = Vector(in1.Dot(Vector(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3],
		in1.Dot(Vector(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3],
		in1.Dot(Vector(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3]);
}

void Math::SmoothAngle( QAngle src, QAngle& dst, float factor ) {
	QAngle delta = dst - src;

	delta.Normalize( );

	dst = src + delta / factor;
}

#define M_RADPI 57.295779513082f
void Math::CalcAngle3(const Vector src, const Vector dst, QAngle& angles) {
	auto delta = src - dst;
	Vector vec_zero = Vector(0.0f, 0.0f, 0.0f);
	QAngle ang_zero = QAngle(0.0f, 0.0f, 0.0f);


	if (delta == vec_zero)
		angles = ang_zero;

	const auto len = delta.Length();

	if (delta.z == 0.0f && len == 0.0f)
		angles = ang_zero;

	if (delta.y == 0.0f && delta.x == 0.0f)
		angles = ang_zero;


#ifdef QUICK_MATH
	angles.x = (fast_asin(delta.z / delta.Length()) * M_RADPI);
	angles.y = (fast_atan(delta.y / delta.x) * M_RADPI);
#else
	angles.x = (asinf(delta.z / delta.Length()) * M_RADPI);
	angles.y = (atanf(delta.y / delta.x) * M_RADPI);
#endif

	angles.z = 0.0f;
	if (delta.x >= 0.0f) { angles.y += 180.0f; }

	angles.Clamp();
}

QAngle Math::CalcAngle( Vector src, Vector dst, bool bruh ) {
	//xd
	if( bruh ) {
		Vector qAngles;
		Vector delta = Vector( ( src[ 0 ] - dst[ 0 ] ), ( src[ 1 ] - dst[ 1 ] ), ( src[ 2 ] - dst[ 2 ] ) );
		double hyp = std::sqrtf( delta[ 0 ] * delta[ 0 ] + delta[ 1 ] * delta[ 1 ] );
		qAngles[ 0 ] = ( float )( std::atan( delta[ 2 ] / hyp ) * ( 180.0 / M_PI ) );
		qAngles[ 1 ] = ( float )( std::atan( delta[ 1 ] / delta[ 0 ] ) * ( 180.0 / M_PI ) );
		qAngles[ 2 ] = 0.f;
		if( delta[ 0 ] >= 0.f )
			qAngles[ 1 ] += 180.f;

		return QAngle( qAngles[ 0 ], qAngles[ 1 ], qAngles[ 2 ] );
	}
	else {
		QAngle angles;
		Vector delta = src - dst;

		angles = delta.ToEulerAngles( );

		angles.Normalize( );

		return angles;
	}
}

Vector Math::VecCalcAngle(const Vector& src, const Vector& dest) {
	Vector angles = Vector(0.0f, 0.0f, 0.0f);
	Vector delta = (src - dest);
	float fHyp = FastRSqrt((delta.x * delta.x) + (delta.y * delta.y));

	angles.x = (atanf(delta.z / fHyp) * RadPi);
	angles.y = (atanf(delta.y / delta.x) * RadPi);
	angles.z = 0.0f;

	if (delta.x >= 0.0f)
		angles.y += 180.0f;

	return angles;
}

Vector Math::GetSmoothedVelocity( float min_delta, Vector a, Vector b ) {
	Vector delta = a - b;
	float delta_length = delta.Length( );

	if( delta_length <= min_delta ) {
		Vector result;
		if( -min_delta <= delta_length ) {
			return a;
		}
		else {
			float iradius = 1.0f / ( delta_length + FLT_EPSILON );
			return b - ( ( delta * iradius ) * min_delta );
		}
	}
	else {
		float iradius = 1.0f / ( delta_length + FLT_EPSILON );
		return b + ( ( delta * iradius ) * min_delta );
	}
}

float Math::AngleDiff( float src, float dst ) {
	float i;

	for( ; src > 180.0; src = src - 360.0 )
		;
	for( ; src < -180.0; src = src + 360.0 )
		;
	for( ; dst > 180.0; dst = dst - 360.0 )
		;
	for( ; dst < -180.0; dst = dst + 360.0 )
		;
	for( i = dst - src; i > 180.0; i = i - 360.0 )
		;
	for( ; i < -180.0; i = i + 360.0 )
		;

	return i;
}

#define CHECK_VALID( _v ) 0

inline void CrossProduct(const Vector& a, const Vector& b, Vector& result)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	Assert(&a != &result);
	Assert(&b != &result);
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
}

void Math::VectorVectors(const Vector& forward, Vector& right, Vector& up)
{
	Vector tmp;

	if (fabs(forward[0]) < 1e-6 && fabs(forward[1]) < 1e-6)
	{
		// pitch 90 degrees up/down from identity
		right[0] = 0;
		right[1] = -1;
		right[2] = 0;
		up[0] = -forward[2];
		up[1] = 0;
		up[2] = 0;
	}
	else
	{
		tmp[0] = 0; tmp[1] = 0; tmp[2] = 1.0;
		CrossProduct(forward, tmp, right);
		right.Normalize();
		CrossProduct(right, forward, up);
		up.Normalize();
	}
}

__forceinline __m128 cos_52s_ps(const __m128 x)
{
	const auto c1 = _mm_set1_ps(0.9999932946f);
	const auto c2 = _mm_set1_ps(-0.4999124376f);
	const auto c3 = _mm_set1_ps(0.0414877472f);
	const auto c4 = _mm_set1_ps(-0.0012712095f);
	const auto x2 = _mm_mul_ps(x, x);
	return _mm_add_ps(c1, _mm_mul_ps(x2, _mm_add_ps(c2, _mm_mul_ps(x2, _mm_add_ps(c3, _mm_mul_ps(c4, x2))))));
}

static const float invtwopi = 0.1591549f;
static const float twopi = 6.283185f;
static const float threehalfpi = 4.7123889f;
static const float halfpi = 1.570796f;
static const __m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

__forceinline void sincos_ps(__m128 angle, __m128* sin, __m128* cos) {
	const auto anglesign = _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(signmask, angle));
	angle = _mm_andnot_ps(signmask, angle);
	angle = _mm_sub_ps(angle, _mm_mul_ps(_mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_mul_ps(angle, _mm_set1_ps(invtwopi)))), _mm_set1_ps(twopi)));

	auto cosangle = angle;
	cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(Math::pi), angle))));
	cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(Math::pi)), signmask));
	cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(threehalfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(twopi), angle))));

	auto result = cos_52s_ps(cosangle);
	result = _mm_xor_ps(result, _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_cmplt_ps(angle, _mm_set1_ps(threehalfpi))), signmask));
	*cos = result;

	const auto sinmultiplier = _mm_mul_ps(anglesign, _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(_mm_cmpgt_ps(angle, _mm_set1_ps(Math::pi)), signmask)));
	*sin = _mm_mul_ps(sinmultiplier, sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.f), _mm_mul_ps(result, result))));
}

void Math::AngleMatrix(const QAngle& angles, matrix3x4_t& matrix)
{
#ifdef _VPROF_MATHLIB
	VPROF_BUDGET("AngleMatrix", "Mathlib");
#endif
	Assert(s_bMathlibInitialized);

	float sr, sp, sy, cr, cp, cy;

#ifdef _X360
	fltx4 radians, scale, sine, cosine;
	radians = LoadUnaligned3SIMD(angles.Base());
	scale = ReplicateX4(M_PI_F / 180.f);
	radians = MulSIMD(radians, scale);
	SinCos3SIMD(sine, cosine, radians);

	sp = SubFloat(sine, 0);	sy = SubFloat(sine, 1);	sr = SubFloat(sine, 2);
	cp = SubFloat(cosine, 0);	cy = SubFloat(cosine, 1);	cr = SubFloat(cosine, 2);
#else
	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);
	SinCos(DEG2RAD(angles.z), &sr, &cr);
#endif

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;

	// NOTE: Do not optimize this to reduce multiplies! optimizer bug will screw this up.
	matrix[0][1] = sr * sp * cy + cr * -sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[2][1] = sr * cp;
	matrix[0][2] = (cr * sp * cy + -sr * -sy);
	matrix[1][2] = (cr * sp * sy + -sr * cy);
	matrix[2][2] = cr * cp;

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

matrix3x4_t Math::AngleMatrix(const QAngle angles)
{
	matrix3x4_t result{};

	m128 angle, sin, cos;
	angle.f[0] = DEG2RAD(angles.x);
	angle.f[1] = DEG2RAD(angles.y);
	angle.f[2] = DEG2RAD(angles.z);
	sincos_ps(angle.v, &sin.v, &cos.v);

	result[0][0] = cos.f[0] * cos.f[1];
	result[1][0] = cos.f[0] * sin.f[1];
	result[2][0] = -sin.f[0];

	const auto crcy = cos.f[2] * cos.f[1];
	const auto crsy = cos.f[2] * sin.f[1];
	const auto srcy = sin.f[2] * cos.f[1];
	const auto srsy = sin.f[2] * sin.f[1];

	result[0][1] = sin.f[0] * srcy - crsy;
	result[1][1] = sin.f[0] * srsy + crcy;
	result[2][1] = sin.f[2] * cos.f[0];

	result[0][2] = sin.f[0] * crcy + srsy;
	result[1][2] = sin.f[0] * crsy - srcy;
	result[2][2] = cos.f[2] * cos.f[0];

	return result;
}

void Math::MatrixCopy(const matrix3x4_t& in, matrix3x4_t& out)
{
	//Assert(s_bMathlibInitialized);
	memcpy(out.Base(), in.Base(), sizeof(float) * 3 * 4);
}

void Math::ConcatTransforms(const matrix3x4_t& in1, const matrix3x4_t& in2, matrix3x4_t& out)
{
	//Assert(s_bMathlibInitialized);
	if (&in1 == &out)
	{
		matrix3x4_t in1b;
		MatrixCopy(in1, in1b);
		ConcatTransforms(in1b, in2, out);
		return;
	}
	if (&in2 == &out)
	{
		matrix3x4_t in2b;
		MatrixCopy(in2, in2b);
		ConcatTransforms(in1, in2b, out);
		return;
	}

	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
		in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
		in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
		in1[2][2] * in2[2][3] + in1[2][3];
}