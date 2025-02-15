////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ��â�� Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

// Direct3D ��ġ ��ü�� ����Ű�� ������ - ������ �۾��� �߽� ����
// �׷��� ��ü�� �����ϰ� ��ȯ�ϰų� ȭ�鿡 �������� �� ���
IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 1024;
const float MIN_VELOCITY = 2;

bool complete = false;   // ����
bool restart = false;    // �絵�� ���� 

// initialize the position (coordinate) of each ball
const float spherePos[36][2] = {
    
    // ��ü Ʋ
    {-1.47f,-4}, {-1.05f,-4}, {-0.63f,-4}, {-0.21f,-4}, {0.21f,-4}, {0.63f,-4}, {1.05f,-4}, {1.47f,-4},
    {-1.89f, -3.58f}, {1.89f, -3.58f},
    {-2.31f, -3.16f}, {-2.31f, -2.74f}, {-2.31f, -2.32f}, {-2.31f, -1.9f}, {-2.31f, -1.48f}, {-2.31f, -1.06f},
    {2.31f, -3.16f}, {2.31f, -2.74f}, {2.31f, -2.32f}, {2.31f, -1.9f}, {2.31f, -1.48f}, {2.31f, -1.06f}, 
    // ��
    {-1.05f,-2.74f}, {-1.05f,-2.32f}, {1.05f,-2.74f}, {1.05f,-2.32f},
    // ��
    {0, -1.48f}, {0, -1.06f},
    // ��
    {-1.47f, -0.64f}, {-1.05f,-0.22f}, {-0.63f,0.2f}, {-0.21f,0.2f}, {0.21f,0.2f}, {0.63f,0.2f}, {1.05f,-0.22f}, {1.47f, -0.64f}
   
};
// initialize the color of each ball
const D3DXCOLOR sphereColor = { d3d::YELLOW };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;   // ��ü�� ȸ��, �̵�, ũ�� ������ ���� ��ȯ ����
D3DXMATRIX g_mView;    // ī�޶� ��ġ �� ���� ���� -> ��鿡�� ��ü�� ���� ���� ����
D3DXMATRIX g_mProj;    // ���� �����̳� ���翵 ������� ȭ�鿡 �׸� �� ���

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982// �ӵ� ������ - ���� / ���� ȿ�� �ùķ��̼� 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
    float               center_x, center_y, center_z;
    float                   m_radius;
    float               m_velocity_x;
    float               m_velocity_z;

public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;   // ��ü�� �׷��� ǥ���� ���� Direct3D �޽� ������
    }
    ~CSphere(void) {}


public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }

    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
        m_velocity_x = 0;
        m_velocity_z = 0;
    }

    // ��ü ������ 
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);  // ���� ��ǥ�踦 ����
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);  // ���� ��ǥ�踦 �߰��� ����
        pDevice->SetMaterial(&m_mtrl);  // ��ü�� ����� ���� ȿ���� ����
        m_pSphereMesh->DrawSubset(0);   // ��ü �޽��� ù ��° ������� �׸�
    }

    bool hasIntersected(CSphere& ball)
    {
        // �� ���� �߽� ��ǥ
        D3DXVECTOR3 center_this = this->getCenter();
        D3DXVECTOR3 center_ball = ball.getCenter();

        // �� ���� ������
        float radius_this = this->getRadius();
        float radius_ball = ball.getRadius();

        // �� �߽� ������ �Ÿ�
        float distanceSquared =
            (center_this.x - center_ball.x) * (center_this.x - center_ball.x) +
            (center_this.y - center_ball.y) * (center_this.y - center_ball.y) +
            (center_this.z - center_ball.z) * (center_this.z - center_ball.z);

        // �浹 ���� �Ǵ�
        float radiusSum = radius_this + radius_ball;
        return distanceSquared <= (radiusSum * radiusSum);
    }

    void hitBy(CSphere& ball)
    {
        if (this->hasIntersected(ball)) {
            // �浹�� ���� �߽� ��ǥ�� ������
            D3DXVECTOR3 redCenter = ball.getCenter();    // ���� ��
            D3DXVECTOR3 yellowCenter = this->getCenter(); // ��� ��
            float yellowRadius = this->getRadius();

            // �߽� �� �Ÿ� ���
            float dx = redCenter.x - yellowCenter.x;
            float dz = redCenter.z - yellowCenter.z;

            // �ݻ� ���� ��� (���� ���ͷ� ����ȭ)
            float magnitude = sqrt(dx * dx + dz * dz);
            float nx = dx / magnitude; // x ������ �ݻ� ����
            float nz = dz / magnitude; // z ������ �ݻ� ����

            // ���� ���� ���� �ӵ� ��������
            float redVelocityX = ball.getVelocity_X();
            float redVelocityZ = ball.getVelocity_Z();

            // �ӵ� ������ �ݻ� ���͸� �������� ����
            float newVelocityX = -2 * nx * (nx * redVelocityX + nz * redVelocityZ) + redVelocityX;
            float newVelocityZ = -2 * nz * (nx * redVelocityX + nz * redVelocityZ) + redVelocityZ;

            ball.setPower(newVelocityX, newVelocityZ);
        }
        else {
            return;
        }
    }

    void ballUpdate(float timeDiff)
    {
        const float TIME_SCALE = 3.3;
        D3DXVECTOR3 cord = this->getCenter();
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());

        if (vx > 0.01 || vz > 0.01)
        {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;

            this->setCenter(tX, cord.y, tZ);
        }

        else {
            this->setPower(0, 0);
        }
        this->setPower(this->getVelocity_X() * DECREASE_RATE, this->getVelocity_Z() * DECREASE_RATE);
        double rate = 1 - (1 - DECREASE_RATE) * timeDiff * 400;
        if (rate < 0)
            rate = 0;

        float newVelocityX = getVelocity_X() * rate;
        float newVelocityZ = getVelocity_Z() * rate;

        float mul = 1.1f;

        if (newVelocityX >= MIN_VELOCITY && newVelocityZ >= MIN_VELOCITY) {
            this->setPower(newVelocityX, newVelocityZ);
        }
        else if (newVelocityX < MIN_VELOCITY && newVelocityZ >= MIN_VELOCITY) {
            this->setPower(newVelocityX * mul, newVelocityZ);
        }
        else if (newVelocityX >= MIN_VELOCITY && newVelocityZ < MIN_VELOCITY) {
            this->setPower(newVelocityX, newVelocityZ * mul);
        }
        else {
            this->setPower(newVelocityX * mul, newVelocityZ * mul);
        }

        // �ӵ��� �ʹ� �������� �ʵ��� �Ѱ谪�� �α�
        float maxSpeed = 5.0f;  // �ִ� �ӵ� (�ʿ信 ���� ����)
        float currentSpeed = sqrt(newVelocityX * newVelocityX + newVelocityZ * newVelocityZ);
        if (currentSpeed > maxSpeed) {
            // �ִ� �ӵ��� ���� �ʵ��� ����
            float speedFactor = maxSpeed / currentSpeed;
            newVelocityX *= speedFactor;
            newVelocityZ *= speedFactor;
            this->setPower(newVelocityX, newVelocityZ);
        }
    }

    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }

    float getRadius(void)  const { return (float)(M_RADIUS); }

    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }

    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }

    void setPower(double vx, double vz)
    {
        this->m_velocity_x = vx;
        this->m_velocity_z = vz;
    }

    void setCenter(float x, float y, float z)
    {
        D3DXMATRIX m;
        center_x = x;   center_y = y;   center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pSphereMesh;

};


// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

    float               m_x;
    float               m_z;
    float                   m_width;
    float                   m_depth;
    float               m_height;

public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        m_width = iwidth;
        m_depth = idepth;

        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pBoundMesh->DrawSubset(0);
    }

    bool hasIntersected(CSphere& ball)
    {
        // ���� �߽� ��ǥ
        D3DXVECTOR3 ballCenter = ball.getCenter();
        float ballRadius = ball.getRadius();

        // ���� ���
        float wallLeft = m_x - m_width / 2.0f;
        float wallRight = m_x + m_width / 2.0f;
        float wallTop = m_z - m_depth / 2.0f;
        float wallBottom = m_z + m_depth / 2.0f;

        // ���� ���� �Ѿ���� Ȯ��
        bool hitX = (ballCenter.x - ballRadius < wallRight) && (ballCenter.x + ballRadius > wallLeft);
        bool hitZ = (ballCenter.z - ballRadius < wallBottom) && (ballCenter.z + ballRadius > wallTop);

        // �浹 ���� ��ȯ
        return hitX && hitZ;
    }

    void hitBy(CSphere& ball) {
        if (this->hasIntersected(ball)) {
            // ���� �߽� ��ǥ �� ������
            D3DXVECTOR3 ballCenter = ball.getCenter();
            float ballRadius = ball.getRadius();

            // ���� ���� ���� 
            const float wallRight = -3.0f;
            const float wallLeft = 3.0f;
            const float wallTop = -4.44f;
            const float wallBottom = 4.44f;

            D3DXVECTOR3 wallNormal = { 0.0f, 0.0f, 0.0f };
            float overlap = 0.0f; // ħ�� �Ÿ�
            int wallType = 0;

            // ���� ����� ���� ħ�� �Ÿ� ���
            if (ballCenter.x <= wallRight + ballRadius) {         // ������ ��
                wallNormal = { 1.0f, 0.0f, 0.0f };
                overlap = wallRight + ballRadius - ballCenter.x;  // X �� ħ�� �Ÿ�
                wallType = 1;
            }
            else if (ballCenter.x >= wallLeft - ballRadius) {     // ���� ��
                wallNormal = { -1.0f, 0.0f, 0.0f };
                overlap = ballCenter.x - (wallLeft - ballRadius); // X �� ħ�� �Ÿ�
                wallType = 1;
            }
            else if (ballCenter.z <= wallTop + ballRadius) {      // ���� ��
                wallNormal = { 0.0f, 0.0f, -1.0f };
                overlap = wallTop + ballRadius - ballCenter.z;    // Z �� ħ�� �Ÿ�
                wallType = 1;
            }
            else if (ballCenter.z >= wallBottom - ballRadius) {   // �Ʒ��� ��
                wallNormal = { 0.0f, 0.0f, 1.0f };
                overlap = ballCenter.z - (wallBottom - ballRadius); // Z �� ħ�� �Ÿ�
                wallType = 2; // �Ʒ��� ���� ��� �߰� ��� ó�� ����
            }

            // ���� �ӵ� ��������
            float velocityX = ball.getVelocity_X();
            float velocityZ = ball.getVelocity_Z();
            D3DXVECTOR3 velocity = { velocityX, 0.0f, velocityZ }; // �ӵ��� 3D ���ͷ� ����

            // ���� �����ߴٸ� �̵� ����
            if (overlap > 0.0f) {
                // ����� �Ÿ� ����
                float penetrationCorrection = overlap / (abs(velocityX) + abs(velocityZ));
                ball.setCenter(
                    ballCenter.x - velocityX * penetrationCorrection,
                    ballCenter.y,
                    ballCenter.z - velocityZ * penetrationCorrection
                );
            }

            // ���� ������ ���� ó��
            switch (wallType) {
            case 0:
                return;
            case 1:
            {
                // �ݻ� ���� ��� (R = V - 2 * (V �� N) * N)
                float dotProduct = velocity.x * wallNormal.x + velocity.z * wallNormal.z;
                D3DXVECTOR3 reflection = {
                    velocity.x - 2 * dotProduct * wallNormal.x,
                    0.0f, // y���� ������ ����
                    velocity.z - 2 * dotProduct * wallNormal.z };

                if (sqrt(reflection.x * reflection.x + reflection.z * reflection.z) < MIN_VELOCITY) {
                    reflection.x *= (MIN_VELOCITY / fabs(reflection.x));
                    reflection.z *= (MIN_VELOCITY / fabs(reflection.z));
                }

                // �� �ӵ��� ���� ����
                ball.setPower(reflection.x, reflection.z);
                break;
            }

            case 2: // �Ʒ��� ��
            {
                restart = true;
                break;
            }
            default:
                printf("error");
                break;
            }
        }
    }

    void setPosition(float x, float y, float z)
    {
        D3DXMATRIX m;
        this->m_x = x;
        this->m_z = z;

        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    float getHeight(void) const { return M_HEIGHT; }

private:
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;

        m_bound._center = lit.Position;
        m_bound._radius = radius;

        m_lit.Type = lit.Type;
        m_lit.Diffuse = lit.Diffuse;
        m_lit.Specular = lit.Specular;
        m_lit.Ambient = lit.Ambient;
        m_lit.Position = lit.Position;
        m_lit.Direction = lit.Direction;
        m_lit.Range = lit.Range;
        m_lit.Falloff = lit.Falloff;
        m_lit.Attenuation0 = lit.Attenuation0;
        m_lit.Attenuation1 = lit.Attenuation1;
        m_lit.Attenuation2 = lit.Attenuation2;
        m_lit.Theta = lit.Theta;
        m_lit.Phi = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;

        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;

        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh* m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;             // �籸�� 
CWall   g_legowall[4];           // �籸���� 4���� ��
std::vector<CSphere> g_sphere;   // �籸�ǿ� �ִ� ��
int spherenum = 0;
CSphere   g_target_redball;        // red ball
CSphere g_whiteball;             // white ball 
CLight   g_light;

bool start = false;

double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

void destroyAllLegoBlock(void)
{
    for (int i = 0; i < spherenum; i++) {
        g_sphere[i].destroy();
    }
    spherenum = 0;
    g_sphere.clear();
    g_target_redball.destroy();
    g_whiteball.destroy();
}

// initialization
bool Setup()
{
    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 9, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 6.24f, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[0].setPosition(0.0f, 0.12f, -4.5f);  // ����
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, 0.3f, 9, d3d::DARKRED)) return false;
    g_legowall[1].setPosition(-3.06f, 0.12f, 0.0f);  // ������
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 9, d3d::DARKRED)) return false;
    g_legowall[2].setPosition(3.06f, 0.12f, 0.0f); // ����
    if (false == g_legowall[3].create(Device, -1, -1, 6.24f, 0.3f, 0.12f, d3d::DARKRED)) return false;
    g_legowall[3].setPosition(0.0f, 0.12f, 4.5f);   // �Ʒ���

    // create balls and set the position
    for (int i = 0;i < 36;i++) {
        g_sphere.push_back(CSphere());
        if (false == g_sphere[i].create(Device, sphereColor)) {
            return false;
        }
        else {
            g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
            g_sphere[i].setPower(0, 0);
            spherenum++;
        }
    }

    // create white and red ball for set direction
    if (false == g_whiteball.create(Device, d3d::WHITE)) return false;
    g_whiteball.setCenter(.0f, (float)M_RADIUS, 4.2f);
    if (false == g_target_redball.create(Device, d3d::RED)) return false;
    g_target_redball.setCenter(g_whiteball.getCenter().x, (float)M_RADIUS, 3.78f);

    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    // Position and aim the camera.
    D3DXVECTOR3 pos(0.0f, 15.0f, 0.0f);   // ī�޶�� Y=10�� ��ġ�� ���̰�, XZ ��鿡�� �߾ӿ� ��ġ��
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f); // ī�޶�� (0, 0, 0)�� ���� �����ٺ� (�籸���� �߾�)
    D3DXVECTOR3 up(0.0f, 0.0f, -1.0f);    // ī�޶��� "����" ������ -Z�� �������� �����Ͽ�, �籸���� �����ٺ����� ��


    D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
    Device->SetTransform(D3DTS_VIEW, &g_mView);

    // Set the projection matrix.
    D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
    Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    g_light.setLight(Device, g_mWorld);

    start = true;

    return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
    for (int i = 0; i < 4; i++) {
        g_legowall[i].destroy();
    }
    destroyAllLegoBlock();
    g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;
    int k = 0;

    if (restart) {
        printf("restart\n");
        Cleanup();
        if (!Setup())
        {
            printf("setup ����\n");
            ::MessageBox(0, "Setup() - FAILED", 0, 0);
            return 0;
        }
        printf("setup�Ϸ�\n");
        restart = false;
        printf("restart = false\n");
        return true;
    }
    else if (complete) {
        printf("success");
        return false;
    }
    if (Device)
    {
        Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
        Device->BeginScene();

        // update the position of each ball. during update, check whether each ball hit by walls.

        D3DXVECTOR3 redcoord = g_target_redball.getCenter();
        D3DXVECTOR3 whitecoord = g_whiteball.getCenter();

        g_target_redball.ballUpdate(timeDelta);
        g_whiteball.ballUpdate(timeDelta);

        if (start) {
            g_target_redball.setCenter(whitecoord.x, redcoord.y, redcoord.z);
        }

        for (k = 0; k < 4; k++) {
            if (g_legowall[k].hasIntersected(g_target_redball)) {
                g_legowall[k].hitBy(g_target_redball);
            }
        }
        if (!restart) {
            // check whether any two balls hit together and update the direction of redball
            for (i = 0;i < spherenum; i++) {
                if (g_sphere[i].hasIntersected(g_target_redball)) {
                    g_sphere[i].hitBy(g_target_redball);
                    g_sphere[i].destroy();
                    g_sphere.erase(g_sphere.begin() + i);
                    spherenum--;
                }
            }

            if (g_whiteball.hasIntersected(g_target_redball)) {
                g_whiteball.hitBy(g_target_redball);
            }
        }
        if (spherenum == 0) {
            complete = true;
        }

        // draw plane, walls, and spheres
        g_legoPlane.draw(Device, g_mWorld);
        for (i = 0;i < 3;i++) {
            g_legowall[i].draw(Device, g_mWorld);
        }
        for (i = 0;i < spherenum;i++) {
            g_sphere[i].draw(Device, g_mWorld);
        }
        g_target_redball.draw(Device, g_mWorld);
        g_whiteball.draw(Device, g_mWorld);
        g_light.draw(Device);

        Device->EndScene();
        Device->Present(0, 0, 0, 0);
        Device->SetTexture(0, NULL);
    }
    return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool wire = false;
    static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

    switch (msg) {
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case VK_ESCAPE:
            ::DestroyWindow(hwnd);    // ESC Ű�� ������ ������ ����
            break;
        case VK_RETURN:
            if (NULL != Device) {
                wire = !wire;
                Device->SetRenderState(D3DRS_FILLMODE,
                    (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));    // Enter Ű�� ���̾������� ��� ��ȯ
            }
            break;
        case VK_SPACE:    // space Ű ������ redball �߻�
            start = false;

            D3DXVECTOR3 targetpos = g_target_redball.getCenter();
            D3DXVECTOR3 whitepos = g_whiteball.getCenter();

            double theta = acos(sqrt(pow(targetpos.x - whitepos.x, 2)) / sqrt(pow(targetpos.x - whitepos.x, 2) +
                pow(targetpos.z - whitepos.z, 2)));      // �⺻ 1 ��и�
            if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x >= 0) { theta = -theta; }   //4 ��и�
            if (targetpos.z - whitepos.z >= 0 && targetpos.x - whitepos.x <= 0) { theta = PI - theta; } //2 ��и�
            if (targetpos.z - whitepos.z <= 0 && targetpos.x - whitepos.x <= 0) { theta = PI + theta; } // 3 ��и�

            double distance = sqrt(pow(targetpos.x - whitepos.x, 2) + pow(targetpos.z - whitepos.z, 2));

            double speedMultiplier = 3;
            g_target_redball.setPower(distance * cos(theta) * speedMultiplier, -distance * sin(theta) * speedMultiplier);

            break;

        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int new_x = LOWORD(lParam);
        int new_y = HIWORD(lParam);
        float dx;
        float dy;

        if (LOWORD(wParam) & MK_LBUTTON) {

            if (isReset) {
                isReset = false;
            }
            else {
                D3DXVECTOR3 vDist;
                D3DXVECTOR3 vTrans;
                D3DXMATRIX mTrans;
                D3DXMATRIX mX;
                D3DXMATRIX mY;

                switch (move) {
                case WORLD_MOVE:
                    dx = (old_x - new_x) * 0.01f;
                    dy = (old_y - new_y) * 0.01f;
                    D3DXMatrixRotationY(&mX, dx);
                    D3DXMatrixRotationX(&mY, dy);
                    g_mWorld = g_mWorld * mX * mY;

                    break;
                }
            }

            old_x = new_x;

        }
        else {
            isReset = true;

            if (LOWORD(wParam) & MK_RBUTTON) {
                dx = (new_x - old_x);// * 0.01f;

                D3DXVECTOR3 whitecoord = g_whiteball.getCenter();

                if ((whitecoord.x + dx * (-0.01f)) < -3.0f + M_RADIUS) {
                    g_whiteball.setCenter(-3.0f + M_RADIUS, whitecoord.y, whitecoord.z);
                }
                else if ((whitecoord.x + dx * (-0.01f)) <= 3.0f - M_RADIUS) {
                    g_whiteball.setCenter(whitecoord.x + dx * (-0.01f), whitecoord.y, whitecoord.z);
                }
                else {
                    g_whiteball.setCenter(3.0f - M_RADIUS, whitecoord.y, whitecoord.z);
                }
            }
            old_x = new_x;

            move = WORLD_MOVE;
        }
        break;
    }
    }
    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
    HINSTANCE prevInstance,
    PSTR cmdLine,
    int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));

    // �ܼ� â �Ҵ�
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);  // printf ����� �ַܼ� ������ ����

    if (!d3d::InitD3D(hinstance,
        Width, Height, true, D3DDEVTYPE_HAL, &Device))
    {
        ::MessageBox(0, "InitD3D() - FAILED", 0, 0);
        return 0;
    }

    if (!Setup())
    {
        ::MessageBox(0, "Setup() - FAILED", 0, 0);
        return 0;
    }

    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}