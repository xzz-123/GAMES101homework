// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(int x, int y, const Vector3f* _v)
{   
	//Eigen::Vector2f p(x, y);
 //   // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
	//std::vector<Vector3f>crossProd(3);
	//for (int i=0;i<3;++i)
	//{
	//	Eigen::Vector2f v1(_v[i].x(), _v[i].y()), v2(_v[(i + 1) % 3].x(), _v[(i + 1) % 3].y());
	//	Eigen::Vector2f side(v2-v1),vec(p-v1);
	//	Vector3f s3(side.x(), side.y(), 1.0), vec_3(vec.x(), vec.y(), 1.0);
	//	crossProd[i] = s3.cross(vec_3);
	//}
	//for (int i=0;i<3;++i)
	//{
	//	if (crossProd[i].dot(crossProd[(i+1)%3])<0)
	//	{
	//		return false;
	//	}
	//}
	//return true;
	Eigen::Vector3f a1 = { _v[0].x() - _v[1].x(),_v[0].y() - _v[1].y(),0 };
	Eigen::Vector3f a2 = { _v[1].x() - _v[2].x(),_v[1].y() - _v[2].y(),0 };
	Eigen::Vector3f a3 = { _v[2].x() - _v[0].x(),_v[2].y() - _v[0].y(),0 };

	Eigen::Vector3f b1 = { x - _v[1].x(),y - _v[1].y(),0 };
	Eigen::Vector3f b2 = { x - _v[2].x(),y - _v[2].y(),0 };
	Eigen::Vector3f b3 = { x - _v[0].x(),y - _v[0].y(),0 };

	float c1, c2, c3;
	c1 = a1.cross(b1).z();
	c2 = a2.cross(b2).z();
	c3 = a3.cross(b3).z();

	if ((c1 >= 0 && c2 >= 0 && c3 >= 0) || (c1 < 0 && c2 < 0 && c3 < 0))return true;
	else return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle

	float l=width, r=0, b=height, top=0;
	for (auto &i:v)
	{
		l = std::min(l, i.x());
		r = std::max(r, i.x());
		b = std::min(b, i.y());
		top = std::max(top, i.y());
	}
	l = floor(l);b = floor(b);
	r = ceil(r);top = ceil(top);
	for (int i=l;i<=r;++i)
	{
		for (int j=b;j<=top;++j)
		{
			if (insideTriangle(i,j,t.v))
			{
				// If so, use the following code to get the interpolated z value.
				auto[alpha, beta, gamma] = computeBarycentric2D(i+0.5, j+0.5, t.v);
				float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
				float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
				z_interpolated *= w_reciprocal;
				// TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
				int ind = get_index(i, j);
				if (z_interpolated<depth_buf[ind])
				{
					set_pixel(Vector3f(i, j, 1.0), t.getColor());
					depth_buf[ind] = z_interpolated;
				}
			}
		}
	}
	//auto v = t.toVector4();
	//int x_max, x_min, y_max, y_min, index;
	//x_min = MIN(floor(v[0].x()), MIN(floor(v[1].x()), floor(v[2].x())));
	//x_max = MAX(ceil(v[0].x()), MAX(ceil(v[1].x()), ceil(v[2].x())));
	//y_min = MIN(floor(v[0].y()), MIN(floor(v[1].y()), floor(v[2].y())));
	//y_max = MAX(ceil(v[0].y()), MAX(ceil(v[1].y()), ceil(v[2].y())));
	//// TODO : Find out the bounding box of current triangle.
	//// iterate through the pixel and find if the current pixel is inside the triangle
	//for (int x = x_min;x <= x_max;x++) {
	//	for (int y = y_min;y <= y_max;y++) {
	//		if (insideTriangle(x + 0.5, y + 0.5, t.v)) {
	//			// If so, use the following code to get the interpolated z value.
	//			auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
	//			float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
	//			float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
	//			z_interpolated *= w_reciprocal;
	//			// TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
	//			index = get_index(x, y);
	//			if (z_interpolated < depth_buf[index]) {
	//				depth_buf[index] = z_interpolated;
	//				set_pixel(Vector3f(x, y, 1), t.getColor());
	//			}
	//		}
	//	}
	//}
	
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on
